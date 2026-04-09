// hcsr04_driver.c
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ktime.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#define TRIGGER_GPIO 529
#define ECHO_GPIO    539

#define INTERRUPT 0

static int major;
static char msg[64];

static unsigned long start_time, end_time;

#if INTERRUPT

int edge_state = 0;
static int irq_number;
int data_ready = 0;
static wait_queue_head_t wq;

static irqreturn_t echo_irq_handler(int irq, void *dev_id)
{
    if (gpio_get_value(ECHO_GPIO)) {
        start_time = ktime_get_ns();   // rising edge
        edge_state = 1;
    } else {
        if(edge_state == 1){
            edge_state = 0;
            end_time = ktime_get_ns();     // falling edge
            data_ready = 1;
            wake_up_interruptible(&wq);
        }
    }

    return IRQ_HANDLED;
}

#endif

static ssize_t hcsr04_read(struct file *file, char __user *buf,
                           size_t len, loff_t *off)
{

#if INTERRUPT

    int ret;
    printk(KERN_INFO "HC-SR04 read Triggered with Interrupt\n");

#else

    printk(KERN_INFO "HC-SR04 read Triggered without Interrupt\n");
    unsigned long start, end;

#endif

    long distance;
    long diff;

    int timeout=0;
#if INTERRUPT
    data_ready = 0;
    edge_state = 0;
#endif

    start_time = 0;
    end_time = 0;

    udelay(60);
    gpio_set_value(TRIGGER_GPIO, 1);
    //printk("Trigger 1 done\n");
    udelay(10);
    gpio_set_value(TRIGGER_GPIO, 0);
    //printk("Trigger 0 done\n");

#if INTERRUPT

    // Wait for IRQ completion (instead of msleep)
    /* -------- Wait for interrupt with timeout -------- */
    ret = wait_event_interruptible_timeout(
        wq,
        data_ready,
        msecs_to_jiffies(300)   // 100 ms timeout
    );

    if (ret == 0) {
        printk(KERN_ERR "HC-SR04 timeout\n");
        return -EIO;
    }

    if (!data_ready) {
        printk(KERN_ERR "HC-SR04 data not ready\n");
        return -EIO;
    }

    /* -------- Calculate distance -------- */
    distance = (end_time - start_time) / 58000;


#else

#if 1
    timeout = 100000;

    //msleep(60);

    while (gpio_get_value(ECHO_GPIO) == 0 && timeout--);

    if(timeout == 0){
        printk("Echo not High received\n");
        return -EIO;
    }

    start = ktime_get_ns();

    timeout = 1000000;

    while (gpio_get_value(ECHO_GPIO) == 1 && timeout--);
    if(timeout == 0){
        printk("Echo not Low received\n");
        return -EIO;
    }
    end = ktime_get_ns();

#else
    u64 start_wait = ktime_get_ns();

    while(gpio_get_value(ECHO_GPIO) == 0 && (ktime_get_ns() - start_wait <= 30000000)){
        //if(ktime_get_ns() - start_wait > 30000000){
            //printk("Echo not High received\n");
            //return -EIO;
        //}
    }

    if(gpio_get_value(ECHO_GPIO) == 0 && (ktime_get_ns() - start_wait > 30000000)){
	printk("Echo not High received\n");
        return -EIO;
    }

    start = ktime_get_ns();
    start_wait = ktime_get_ns();

    while(gpio_get_value(ECHO_GPIO) == 1 && (ktime_get_ns() - start_wait <= 30000000)){
        //if(ktime_get_ns() - start_wait > 30000000){
            //printk("Echo not Low received\n");
            //return -EIO;
        //}
    }

    if(gpio_get_value(ECHO_GPIO) == 1 && (ktime_get_ns() - start_wait > 30000000)){
        printk("Echo not Low received\n");
        return -EIO;
    }

    end = ktime_get_ns();
#endif
    diff = end - start;
    distance = (diff * 100) / 58000;

#endif

    snprintf(msg, sizeof(msg), "%ld.%02ld\n", distance / 100, distance % 100);

    return simple_read_from_buffer(buf, len, off, msg, strlen(msg));
}

// ---------------- OPEN ----------------
static int hcsr04_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "HC-SR04 device opened\n");
    return 0;
}

// ---------------- RELEASE ----------------
static int hcsr04_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "HC-SR04 device closed\n");
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = hcsr04_read,
    .open = hcsr04_open,
    .release = hcsr04_release,
};

static int __init hcsr04_init(void)
{

    int ret;

#if INTERRUPT
    init_waitqueue_head(&wq);
#endif
    if (gpio_request(TRIGGER_GPIO, "trigger")) {
        printk(KERN_ERR "Failed to request TRIGGER GPIO\n");
        return -1;
    }

    if (gpio_request(ECHO_GPIO, "echo")) {
        printk(KERN_ERR "Failed to request ECHO GPIO\n");
        gpio_free(TRIGGER_GPIO);
        return -1;
    }

    gpio_direction_output(TRIGGER_GPIO, 0);
    gpio_direction_input(ECHO_GPIO);

#if INTERRUPT

    irq_number = gpio_to_irq(ECHO_GPIO);

    ret = request_irq(irq_number,
            echo_irq_handler,
            IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
            "hcsr04_echo",
            NULL);

    if(ret){
        printk("IRQ Request Failed");
        gpio_free(TRIGGER_GPIO);
        gpio_free(ECHO_GPIO);
        return -1;
    }

#endif

    major = register_chrdev(0, "hcsr04", &fops);

    if (major < 0) {
        printk(KERN_ERR "Failed to register char device\n");
#if INTERRUPT
        free_irq(irq_number, NULL);
#endif
        gpio_free(TRIGGER_GPIO);
        gpio_free(ECHO_GPIO);
        return -1;
    }

    printk(KERN_INFO "HC-SR04 driver loaded\n");
    return 0;
}

static void __exit hcsr04_exit(void)
{
    unregister_chrdev(major, "hcsr04");
    gpio_free(TRIGGER_GPIO);
    gpio_free(ECHO_GPIO);
#if INTERRUPT
    free_irq(irq_number, NULL);
#endif
    printk(KERN_INFO "HC-SR04 driver removed\n");
}

module_init(hcsr04_init);
module_exit(hcsr04_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shiva");
MODULE_DESCRIPTION("Simple HC-SR03 GPIO driver");
