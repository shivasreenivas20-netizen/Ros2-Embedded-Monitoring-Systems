#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#define DRIVER_NAME "mcp3008"
#define CLASS_NAME  "mcp3008_class"

/* IOCTL Commands */
#define SET_CHANNEL _IOW('a', 1, int)
#define GET_VALUE   _IOR('a', 2, int)

static struct spi_device *spi_dev;
static int current_channel = 0;

static dev_t dev_num;
static struct cdev mcp_cdev;
static struct class *mcp_class;

/* -------- SPI READ FUNCTION -------- */
static int mcp3008_read_channel(int ch)
{
	u8 tx[3] = {1, (8 + ch) << 4, 0};
	u8 rx[3] = {0};

	printk("TX: %x %x %x\n", tx[0], tx[1], tx[2]);

	struct spi_transfer t = {
	    	.tx_buf = tx,
		.rx_buf = rx,
		.len = 3,
		.speed_hz = 1000000,   // IMPORTANT
    		.bits_per_word = 8,
    		//.delay_usecs = 0,
    		.cs_change = 0,
	};

	struct spi_message m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	if (spi_sync(spi_dev, &m) < 0) {
    		printk("SPI transfer failed\n");
    		return -EIO;
	}

	printk("RX: %x %x %x\n", rx[0], rx[1], rx[2]);

	return ((rx[1] & 3) << 8) | rx[2];
}

/* -------- READ (cat support) -------- */
static ssize_t mcp_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
	char out[32];
	int val;

	val = mcp3008_read_channel(current_channel);

	int ret = snprintf(out, sizeof(out),
                   "CH%d=%d\n", current_channel, val);

	return simple_read_from_buffer(buf, len, off, out, ret);
}

/* -------- IOCTL -------- */
static long mcp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ch, val;

	switch (cmd) {

    		case SET_CHANNEL:
        		if (copy_from_user(&ch, (int *)arg, sizeof(ch)))
            			return -EFAULT;

        		if (ch < 0 || ch > 7)
            			return -EINVAL;

        		current_channel = ch;
        		printk("Channel set to %d\n", ch);
        		break;

    		case GET_VALUE:
        		val = mcp3008_read_channel(current_channel);

        		if (copy_to_user((int *)arg, &val, sizeof(val)))
            			return -EFAULT;

        		break;

    		default:
        		return -EINVAL;
	}

	return 0;
}

/* -------- FILE OPS -------- */
static struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = mcp_read,
	.unlocked_ioctl = mcp_ioctl,
};

/* -------- PROBE -------- */
static int mcp_probe(struct spi_device *spi)
{
	spi_dev = spi;

	printk("MCP3008 SPI Driver Loaded\n");

	/* SPI Config */
	spi->mode = SPI_MODE_0;
	spi->bits_per_word = 8;
	spi->max_speed_hz = 1000000;
	spi_setup(spi);

	/* Char device */
	alloc_chrdev_region(&dev_num, 0, 1, DRIVER_NAME);

	cdev_init(&mcp_cdev, &fops);
	cdev_add(&mcp_cdev, dev_num, 1);

	mcp_class = class_create(CLASS_NAME);
	device_create(mcp_class, NULL, dev_num, NULL, "mcp3008");

	return 0;
}

/* -------- REMOVE -------- */
static void mcp_remove(struct spi_device *spi)
{
	device_destroy(mcp_class, dev_num);
	class_destroy(mcp_class);
	cdev_del(&mcp_cdev);
	unregister_chrdev_region(dev_num, 1);
	printk("MCP3008 Removed\n");
}

/* -------- ID -------- */
static const struct spi_device_id mcp_id[] = {
	{ "mcp3008", 0 },
	{}
};
MODULE_DEVICE_TABLE(spi, mcp_id);

/* -------- DRIVER -------- */
static struct spi_driver mcp_driver = {
	.driver = {
		.name = DRIVER_NAME,
	},
	.probe = mcp_probe,
	.remove = mcp_remove,
	.id_table = mcp_id,
};

module_spi_driver(mcp_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shiva Kumar");
MODULE_DESCRIPTION("MCP3008 SPI Driver (read + ioctl)");
