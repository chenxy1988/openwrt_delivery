/*
 * SFP Driver for BMU PL interface
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/mutex.h>
#include <linux/of_net.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_net.h>



#define DRV_NAME "sfp-ethernet"



struct sfp_platform_data {
        u8 macaddr[ETH_ALEN];
        int rx_dma_channel;
        int tx_dma_channel;
};

#define DMA_BUFFER_SIZE		2048

struct sfp_tx_dma_ctl {
	struct dma_chan *chan;
	struct dma_async_tx_descriptor *adesc;
	void *buf;
	struct scatterlist sg;
	dma_addr_t dma_handle;
	dma_cookie_t cookie;
	struct completion cmp;
	int channel;
};

struct sfp_rx_dma_ctl {
	struct dma_chan *chan;
	struct dma_async_tx_descriptor *adesc;
	struct sk_buff  *skb;
	struct scatterlist sg;
	struct tasklet_struct tasklet;
	dma_cookie_t cookie;
	dma_addr_t dma_handle;
	struct completion cmp;
	int channel;
};

struct sfp_adapter {
	unsigned long	conf_flags;
	struct tasklet_struct	tasklet;
	spinlock_t	lock;
	struct work_struct timeout_work;
	struct net_device *netdev;
	struct device *dev;
	struct sfp_tx_dma_ctl	dma_tx;
	struct sfp_rx_dma_ctl	dma_rx;
};

static void sfp_dma_rx_cb(void *data);
static void sfp_dma_tx_cb(void *data);

static void sfp_set_link_status(struct net_device *netdev,int status)
{
	if(status == 1){
                netif_carrier_on(netdev);
                netif_wake_queue(netdev);
	}else{
                netif_stop_queue(netdev);
                netif_carrier_off(netdev);
	}
}

static void sfp_update_link_status(struct net_device *netdev,
	struct sfp_adapter *adapter)
{
	sfp_set_link_status(netdev,0);
	sfp_set_link_status(netdev,1);
}


static int sfp_tx_frame_dma(struct sk_buff *skb, struct net_device *netdev)
{
	struct sfp_adapter *adapter = netdev_priv(netdev);
	struct sfp_tx_dma_ctl *ctl = &adapter->dma_tx;
	u8 *buf = ctl->buf;

	if (ctl->adesc) {
		netdev_dbg(netdev, "%s: TX ongoing\n", __func__);
		return NETDEV_TX_BUSY;
	}

	ctl->dma_handle = dma_map_single(adapter->dev,
                ctl->buf, DMA_BUFFER_SIZE, DMA_TO_DEVICE);


        sg_init_table(&ctl->sg,1);
        sg_dma_address(&ctl->sg) =ctl->dma_handle;
        sg_dma_len(&ctl->sg) = skb->len;


	sg_dma_len(&ctl->sg) = skb->len;

	skb_copy_from_linear_data(skb, buf, skb->len);
	dma_sync_single_range_for_device(adapter->dev,
		sg_dma_address(&ctl->sg), 0, sg_dma_len(&ctl->sg),
		DMA_TO_DEVICE);

	if (sg_dma_len(&ctl->sg) % 4)
		sg_dma_len(&ctl->sg) += 4 - sg_dma_len(&ctl->sg) % 4;

	ctl->adesc = dmaengine_prep_slave_sg(ctl->chan,
		&ctl->sg, 1, DMA_MEM_TO_DEV, DMA_PREP_INTERRUPT);

	if (!ctl->adesc){
		dma_unmap_single(adapter->dev,
 	               ctl->dma_handle, DMA_BUFFER_SIZE, DMA_TO_DEVICE);
		return NETDEV_TX_BUSY;
	}

	ctl->adesc->callback_param = netdev;
	ctl->adesc->callback = sfp_dma_tx_cb;
	ctl->adesc->tx_submit(ctl->adesc);

	netdev->stats.tx_bytes += skb->len;

	dev_kfree_skb(skb);

	return NETDEV_TX_OK;
}


static void sfp_update_rx_counters(struct net_device *netdev, u32 status,
	int len)
{
	netdev_dbg(netdev, "RX packet, len: %d\n", len);

	netdev->stats.rx_packets++;
	netdev->stats.rx_bytes += len;
}

static int __sfp_start_new_rx_dma(struct net_device *netdev)
{
	struct sfp_adapter *adapter = netdev_priv(netdev);
	struct sfp_rx_dma_ctl *ctl = &adapter->dma_rx;
	struct scatterlist *sg = &ctl->sg;
	int err;

	ctl->skb = netdev_alloc_skb(netdev, DMA_BUFFER_SIZE);
	if (ctl->skb) {
		sg_init_table(sg, 1);
		sg_dma_address(sg) = dma_map_single(adapter->dev,
			ctl->skb->data, DMA_BUFFER_SIZE, DMA_FROM_DEVICE);
		if (dma_mapping_error(adapter->dev, sg_dma_address(sg))) {
			err = -ENOMEM;
			sg_dma_address(sg) = 0;
			goto out;
		}

		sg_dma_len(sg) = DMA_BUFFER_SIZE;

		ctl->adesc = dmaengine_prep_slave_sg(ctl->chan,
			sg, 1, DMA_DEV_TO_MEM, DMA_PREP_INTERRUPT);

		if (!ctl->adesc) {
			err = -ENOMEM;
			goto out;
		}

		ctl->adesc->callback_param = netdev;
		ctl->adesc->callback = sfp_dma_rx_cb;
		ctl->adesc->tx_submit(ctl->adesc);
		dma_async_issue_pending(ctl->chan);
	} else {
		err = -ENOMEM;
		sg_dma_address(sg) = 0;
		goto out;
	}

	return 0;
out:
	if (sg_dma_address(sg))
		dma_unmap_single(adapter->dev, sg_dma_address(sg),
			DMA_BUFFER_SIZE, DMA_FROM_DEVICE);
	sg_dma_address(sg) = 0;
	if (ctl->skb)
		dev_kfree_skb(ctl->skb);

	ctl->skb = NULL;

	printk(KERN_ERR DRV_NAME": Failed to start RX DMA: %d\n", err);
	return err;
}

static void sfp_rx_frame_dma_tasklet(unsigned long arg)
{
	struct net_device *netdev = (struct net_device *)arg;
	struct sfp_adapter *adapter = netdev_priv(netdev);
	struct sfp_rx_dma_ctl *ctl = &adapter->dma_rx;
	struct sk_buff *skb = ctl->skb;
	dma_addr_t addr = sg_dma_address(&ctl->sg);
	u32 status; u32 len=1600;

	ctl->adesc = NULL;

	__sfp_start_new_rx_dma(netdev);

	dma_unmap_single(adapter->dev, addr, DMA_BUFFER_SIZE, DMA_FROM_DEVICE);

	status = *((u32 *)skb->data);

	netdev_dbg(netdev, "%s - rx_data: status: %x\n",
		__func__, status & 0xffff);

	sfp_update_rx_counters(netdev, status, len);

	skb_put(skb, len);

	skb->protocol = eth_type_trans(skb, netdev);
	netif_rx(skb);
}


static void sfp_tasklet(unsigned long arg)
{
	struct net_device *netdev = (struct net_device *)arg;

	if (!netif_running(netdev))
		return;

}


static void sfp_dma_rx_cb(void *data)
{
	struct net_device	*netdev = data;
	struct sfp_adapter	*adapter = netdev_priv(netdev);

	netdev_dbg(netdev, "RX DMA finished\n");
	if (adapter->dma_rx.adesc)
		tasklet_schedule(&adapter->dma_rx.tasklet);
}

static void sfp_dma_tx_cb(void *data)
{
	struct net_device		*netdev = data;
	struct sfp_adapter		*adapter = netdev_priv(netdev);
	struct sfp_tx_dma_ctl	*ctl = &adapter->dma_tx;


	netdev_dbg(netdev, "TX DMA finished\n");

	if (!ctl->adesc)
		return;

	netdev->stats.tx_packets++;
	ctl->adesc = NULL;

	dma_unmap_single(adapter->dev,
                       ctl->dma_handle, DMA_BUFFER_SIZE, DMA_TO_DEVICE);

	if (netif_queue_stopped(netdev)){
		netif_wake_queue(netdev);
	}
}

static void sfp_stop_dma(struct sfp_adapter *adapter)
{
	struct sfp_tx_dma_ctl *tx_ctl = &adapter->dma_tx;
	struct sfp_rx_dma_ctl *rx_ctl = &adapter->dma_rx;

	tx_ctl->adesc = NULL;
	if (tx_ctl->chan)
		dmaengine_terminate_all(tx_ctl->chan);

	rx_ctl->adesc = NULL;
	if (rx_ctl->chan)
		dmaengine_terminate_all(rx_ctl->chan);

	if (sg_dma_address(&rx_ctl->sg))
		dma_unmap_single(adapter->dev, sg_dma_address(&rx_ctl->sg),
			DMA_BUFFER_SIZE, DMA_FROM_DEVICE);
	sg_dma_address(&rx_ctl->sg) = 0;

	dev_kfree_skb(rx_ctl->skb);
	rx_ctl->skb = NULL;
}

static void sfp_dealloc_dma_bufs(struct sfp_adapter *adapter)
{
	struct sfp_tx_dma_ctl *tx_ctl = &adapter->dma_tx;
	struct sfp_rx_dma_ctl *rx_ctl = &adapter->dma_rx;

	sfp_stop_dma(adapter);

	if (tx_ctl->chan)
		dma_release_channel(tx_ctl->chan);
	tx_ctl->chan = NULL;

	if (rx_ctl->chan)
		dma_release_channel(rx_ctl->chan);
	rx_ctl->chan = NULL;

	tasklet_kill(&rx_ctl->tasklet);

	if (sg_dma_address(&tx_ctl->sg))
		dma_unmap_single(adapter->dev, sg_dma_address(&tx_ctl->sg),
			DMA_BUFFER_SIZE, DMA_TO_DEVICE);
	sg_dma_address(&tx_ctl->sg) = 0;

	kfree(tx_ctl->buf);
	tx_ctl->buf = NULL;
}

static int sfp_alloc_dma_bufs(struct net_device *netdev)
{
	struct sfp_adapter *adapter = netdev_priv(netdev);
	struct sfp_tx_dma_ctl *tx_ctl = &adapter->dma_tx;
	struct sfp_rx_dma_ctl *rx_ctl = &adapter->dma_rx;
	int err;

	sg_init_table(&tx_ctl->sg, 1);

	tx_ctl->chan =dma_request_slave_channel(adapter->dev, "axidma0");
	if (!tx_ctl->chan) {
		printk("Failed to request slave_channel for tx \n");
		err = -ENODEV;
		goto err;
	}

	tx_ctl->buf = kmalloc(DMA_BUFFER_SIZE, GFP_KERNEL);
	if (!tx_ctl->buf) {
		printk("Failed alloc memory for TX direction \n");
		err = -ENOMEM;
		goto err;
	}


	rx_ctl->chan = dma_request_slave_channel(adapter->dev, "axidma1");

	if (!rx_ctl->chan) {
		printk(":Failed to request rx channel \n");
		err = -ENODEV;
		goto err;
	}

	tasklet_init(&rx_ctl->tasklet, sfp_rx_frame_dma_tasklet,
		(unsigned long)netdev);

	return 0;
err:
	sfp_dealloc_dma_bufs(adapter);
	return err;
}

static int sfp_open(struct net_device *netdev)
{
	struct sfp_adapter *adapter = netdev_priv(netdev);
	int err;

	netdev_dbg(netdev, "%s - entry\n", __func__);
	err = sfp_alloc_dma_bufs(netdev);

	if (!err) {

		err = __sfp_start_new_rx_dma(netdev);
		if (err)
			sfp_dealloc_dma_bufs(adapter);
	}

	if (err) {
		printk(KERN_WARNING DRV_NAME
			": Failed to initiate DMA, running PIO\n");
		sfp_dealloc_dma_bufs(adapter);
			adapter->dma_rx.channel = -1;
			adapter->dma_tx.channel = -1;
	}

	sfp_update_link_status(netdev, adapter);


	return 0;
}

static int sfp_close(struct net_device *netdev)
{
	struct sfp_adapter *adapter = netdev_priv(netdev);

	netdev_dbg(netdev, "%s - entry\n", __func__);

	cancel_work_sync(&adapter->timeout_work);

	sfp_dealloc_dma_bufs(adapter);

	return 0;
}

static netdev_tx_t sfp_xmit_frame(struct sk_buff *skb,
				     struct net_device *netdev)
{
	int ret;
	unsigned long flags;
	struct sfp_adapter *adapter = netdev_priv(netdev);
        struct sfp_tx_dma_ctl *ctl = &adapter->dma_tx;

	netdev_dbg(netdev, "%s: entry\n", __func__);
	spin_lock_irqsave(&adapter->lock, flags);
	ret = sfp_tx_frame_dma(skb, netdev);

        if (adapter->dma_tx.adesc){
        	netif_stop_queue(netdev);
		dma_async_issue_pending(ctl->chan);
	}
	spin_unlock_irqrestore(&adapter->lock, flags);

	return ret;

}

static int sfp_set_mac(struct net_device *netdev, void *p)
{
	struct sockaddr *addr = p;
	char *mac = (u8 *)addr->sa_data;

	netdev_dbg(netdev, "%s: entry\n", __func__);

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;

	memcpy(netdev->dev_addr, mac, netdev->addr_len);

	return 0;
}

static void sfp_tx_timeout_work(struct work_struct *work)
{
	struct sfp_adapter *adapter =
		container_of(work, struct sfp_adapter, timeout_work);
	struct net_device *netdev = adapter->netdev;

	sfp_dealloc_dma_bufs(adapter);
	sfp_alloc_dma_bufs(netdev);
	netif_wake_queue(netdev);
	netdev_err(netdev, "%s: entry\n", __func__);
}

static void sfp_tx_timeout(struct net_device *netdev)
{
	struct sfp_adapter *adapter = netdev_priv(netdev);

	netdev_dbg(netdev, "%s: entry\n", __func__);

	schedule_work(&adapter->timeout_work);
}

static const struct net_device_ops sfp_netdev_ops = {
	.ndo_open		= sfp_open,
	.ndo_stop		= sfp_close,
	.ndo_start_xmit		= sfp_xmit_frame,
	.ndo_set_mac_address	= sfp_set_mac,
	.ndo_tx_timeout 	= sfp_tx_timeout,
	.ndo_validate_addr	= eth_validate_addr
};

static const struct ethtool_ops sfp_ethtool_ops = {
	.get_link		= ethtool_op_get_link,
};

static int sfp_probe(struct platform_device *pdev)
{
	int err = -ENOMEM;
	struct net_device *netdev;
	struct sfp_adapter *adapter;
	struct sfp_platform_data *pdata = dev_get_platdata(&pdev->dev);
	unsigned i;

	netdev = alloc_etherdev(sizeof(struct sfp_adapter));
	if (!netdev)
		return err;

	SET_NETDEV_DEV(netdev, &pdev->dev);

	adapter = netdev_priv(netdev);
	adapter->netdev = netdev;
	INIT_WORK(&adapter->timeout_work, sfp_tx_timeout_work);

	adapter->dev = &pdev->dev;

	tasklet_init(&adapter->tasklet, sfp_tasklet, (unsigned long)netdev);
	spin_lock_init(&adapter->lock);

	netdev->netdev_ops = &sfp_netdev_ops;
	netdev->ethtool_ops = &sfp_ethtool_ops;

	i = netdev->addr_len;
	if (pdata) {
		for (i = 0; i < netdev->addr_len; i++)
			if (pdata->macaddr[i] != 0)
				break;

		if (i < netdev->addr_len)
			memcpy(netdev->dev_addr, pdata->macaddr,
				netdev->addr_len);
	}

	netdev->dev_addr = (void *)of_get_mac_address(pdev->dev.of_node);
	

	if (!is_valid_ether_addr(netdev->dev_addr))
		eth_hw_addr_random(netdev);


	strcpy(netdev->name, "sfp%d");
        netdev->min_mtu = 64;
	err = register_netdev(netdev);
	if (err)
		goto err_register;

	platform_set_drvdata(pdev, netdev);

	return 0;

err_register:
	free_netdev(netdev);
	return err;
}

static int sfp_remove(struct platform_device *pdev)
{
	struct net_device *netdev = platform_get_drvdata(pdev);
	struct sfp_adapter *adapter = netdev_priv(netdev);

	unregister_netdev(netdev);
	tasklet_kill(&adapter->tasklet);
	free_netdev(netdev);
	return 0;
}

static const struct of_device_id dma_proxy_of_ids[] = {
        { .compatible = "xlnx,axi-sfp-ethernet-1.00.a",},
        {}
};


static struct platform_driver sfp_platform_driver = {
	.driver = {
		.name	= DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = dma_proxy_of_ids,
	},
	.probe		= sfp_probe,
	.remove		= sfp_remove,
};

static int __init sfp_ethernet_init(void)
{

	platform_driver_register(&sfp_platform_driver);
	printk("S.\
		F.\
		P.\
		E.\
		T.\
		H.\
		E.\
		R.\
		N.\
		E.\
		T \n");

	return 0;
}

static void __exit sfp_ethernet_exit(void)
{
	printk("S.\
		F.\
		P.\
		E.\
		X.\
		I.\
		T.\n");

	platform_driver_unregister(&sfp_platform_driver);

}

module_init(sfp_ethernet_init);
module_exit(sfp_ethernet_exit);

MODULE_DESCRIPTION("SFP Ethernet driver for zynq platform");
MODULE_AUTHOR("CHEN Xiangyu <xiangyu.chen@aol.com>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:sfp-eth");

