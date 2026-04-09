#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#include <linux/ioctl.h>

#define BMP_IOC_MAGIC  'b'
#define GET_TEMP     _IOR(BMP_IOC_MAGIC, 1, int)
#define GET_PRESSURE _IOR(BMP_IOC_MAGIC, 2, unsigned int)

#define DRIVER_NAME "bmp280"
#define CLASS_NAME  "bmp280_class"

struct bmp_data
{
	struct i2c_client *client;

	u16 dig_T1;
	s16 dig_T2, dig_T3;

	u16 dig_P1;
	s16 dig_P2, dig_P3, dig_P4;
	s16 dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;

	int t_fine;
};

static struct bmp_data bmp;
static dev_t dev_num;
static struct cdev bmp_cdev;
static struct class *bmp_class;

/* -------- I2C Helpers -------- */
static int read_bytes(u8 reg, u8 *buf, int len)
{
	return i2c_smbus_read_i2c_block_data(bmp.client, reg, len, buf);
}

/* -------- Calibration -------- */
static void read_calibration(void)
{
	u8 buf[24];

	read_bytes(0x88, buf, 24);

	bmp.dig_T1 = buf[1]<<8 | buf[0];
	bmp.dig_T2 = buf[3]<<8 | buf[2];
	bmp.dig_T3 = buf[5]<<8 | buf[4];

	bmp.dig_P1 = buf[7]<<8 | buf[6];
	bmp.dig_P2 = buf[9]<<8 | buf[8];
	bmp.dig_P3 = buf[11]<<8 | buf[10];
	bmp.dig_P4 = buf[13]<<8 | buf[12];
	bmp.dig_P5 = buf[15]<<8 | buf[14];
	bmp.dig_P6 = buf[17]<<8 | buf[16];
	bmp.dig_P7 = buf[19]<<8 | buf[18];
	bmp.dig_P8 = buf[21]<<8 | buf[20];
	bmp.dig_P9 = buf[23]<<8 | buf[22];
}

/* -------- Raw Read -------- */
static void read_raw(int *adc_T, int *adc_P)
{
	u8 buf[6];
	read_bytes(0xF7, buf, 6);

	*adc_P = (buf[0]<<12)|(buf[1]<<4)|(buf[2]>>4);
	*adc_T = (buf[3]<<12)|(buf[4]<<4)|(buf[5]>>4);
}

/* -------- Compensation -------- */
static int compensate_temp(int adc_T)
{
	int var1 = ((((adc_T>>3) - (bmp.dig_T1<<1))) * bmp.dig_T2) >> 11;
	int var2 = (((((adc_T>>4) - bmp.dig_T1) * ((adc_T>>4) - bmp.dig_T1)) >> 12) * bmp.dig_T3) >> 14;

	bmp.t_fine = var1 + var2;
	return (bmp.t_fine * 5 + 128) >> 8;  // returns temp * 100
}

static unsigned int compensate_pressure(int adc_P)
{
	long long var1, var2, p;

	var1 = ((long long)bmp.t_fine) - 128000;
	var2 = var1 * var1 * bmp.dig_P6;
	var2 = var2 + ((var1 * bmp.dig_P5) << 17);
	var2 = var2 + (((long long)bmp.dig_P4) << 35);
	var1 = ((var1 * var1 * bmp.dig_P3) >> 8) + ((var1 * bmp.dig_P2) << 12);
	var1 = (((((long long)1) << 47) + var1)) * bmp.dig_P1 >> 33;

	if (var1 == 0)
		return 0;

	p = 1048576 - adc_P;
	p = (((p << 31) - var2) * 3125) / var1;
	var1 = (bmp.dig_P9 * (p >> 13) * (p >> 13)) >> 25;
	var2 = (bmp.dig_P8 * p) >> 19;

	p = ((p + var1 + var2) >> 8) + (((long long)bmp.dig_P7) << 4);

	return (unsigned int)(p / 256); // Pa
}

/* -------- IOCTL -------- */
static long bmp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int adc_T, adc_P;
	int temp;
	unsigned int pressure;

	read_raw(&adc_T, &adc_P);

	temp = compensate_temp(adc_T);
	pressure = compensate_pressure(adc_P);

        printk("%d %d\n",temp,pressure);

	switch (cmd) {
		case GET_TEMP:
        		return copy_to_user((int *)arg, &temp, sizeof(temp)) ? -EFAULT : 0;

		case GET_PRESSURE:
        		return copy_to_user((unsigned int *)arg, &pressure, sizeof(pressure)) ? -EFAULT : 0;

		default:
        		return -EINVAL;
	}
}

/* -------- Read --------- */
static ssize_t bmp_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
	char out[64];
	int adc_T, adc_P;
	int temp;
	unsigned int pressure;
	int ret;

	/* Read raw data */
	read_raw(&adc_T, &adc_P);

	/* Apply compensation */
	temp = compensate_temp(adc_T);
	pressure = compensate_pressure(adc_P);

        printk("%d %d\n",temp,pressure);

	/* Format output */
	ret = snprintf(out, sizeof(out), "Temp=%d.%02d C Pressure=%d.%02d hPa\n", temp / 100, temp % 100, pressure / 100, pressure % 100);

	/* Copy to user safely */
	return simple_read_from_buffer(buf, len, offset, out, ret);
}

/* -------- File Ops -------- */
static struct file_operations fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = bmp_ioctl,
	.read = bmp_read,
};

/* -------- Probe -------- */
static int bmp_probe(struct i2c_client *client)
{
	bmp.client = client;

	printk("BMP280 ID: 0x%x\n", i2c_smbus_read_byte_data(client, 0xD0));

	read_calibration();

	/* set normal mode */
	i2c_smbus_write_byte_data(client, 0xF4, 0x27);

	alloc_chrdev_region(&dev_num, 0, 1, DRIVER_NAME);
	cdev_init(&bmp_cdev, &fops);
	cdev_add(&bmp_cdev, dev_num, 1);

	bmp_class = class_create(CLASS_NAME);
	device_create(bmp_class, NULL, dev_num, NULL, "bmp280");

	printk("BMP280 Driver Ready\n");
	return 0;

}

/* -------- Remove -------- */
static void bmp_remove(struct i2c_client *client)
{
	device_destroy(bmp_class, dev_num);
	class_destroy(bmp_class);
	cdev_del(&bmp_cdev);
	unregister_chrdev_region(dev_num, 1);
}

/* -------- Driver -------- */
static const struct i2c_device_id bmp_id[] = {
	{ "bmp280_char", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, bmp_id);

static struct i2c_driver bmp_driver = {
	.driver = { .name = "bmp280_char" },
	.probe = bmp_probe,
	.remove = bmp_remove,
	.id_table = bmp_id,
};

module_i2c_driver(bmp_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("BMP280 I2C Driver");
