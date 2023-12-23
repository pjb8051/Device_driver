#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/fs.h>          
#include <linux/errno.h>       
#include <linux/types.h>       
#include <linux/fcntl.h>       

#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/timer.h>
#include <linux/time.h>

#include "ioctl_test.h"

#define LEDKEY_NAME	"ledkeydev"
#define LEDKEY_MAJOR	230      
#define OFF				0
#define ON 				1
#define GPIOLEDCNT		8
#define GPIOKEYCNT		8

#define DEBUG 1

typedef struct {
	int key_irq[8];
	int keyNumber;
} keyData;

static int gpioLed[GPIOLEDCNT] = {6,7,8,9,10,11,12,13};
static int gpioKey[GPIOKEYCNT] = {16,17,18,19,20,21,22,23};

static int gpioLedInit(void);
static void gpioLedSet(long);
static void gpioLedFree(void);
static int gpioKeyInit(void);
//static int gpioKeyGet(void);
static void gpioKeyFree(void);
static int gpioKeyIrq(keyData *);
static void gpioKeyIrqFree(keyData *);
static int requestIrqInit(keyData *);

static int timerVal = 100;
module_param(timerVal, int, 0);
static int ledVal = 0;
module_param(ledVal, int, 0);
struct timer_list timerLed;

static void kerneltimer_func(struct timer_list *t );

DECLARE_WAIT_QUEUE_HEAD(WaitQueue_Read);

static void kerneltimer_registertimer(unsigned long timeover)
{
    timer_setup( &timerLed,kerneltimer_func,0);
    timerLed.expires = get_jiffies_64() + timeover;  //10ms *100 = 1sec
    add_timer( &timerLed );
}

static void kerneltimer_func(struct timer_list *t)
{
#if DEBUG
    printk("ledVal : %#04x\n",(unsigned int)(ledVal));
#endif
    ledVal = ~ledVal & 0xff;
    gpioLedSet(ledVal);
    mod_timer(t,get_jiffies_64() + timerVal);
}


irqreturn_t key_isr(int irq, void *pData)
{
	int i;
	keyData *pKeyData = (keyData *)pData;
	for(i = 0; i < GPIOKEYCNT; i++)
	{
		if(irq == pKeyData->key_irq[i])
		{
			pKeyData->keyNumber = i+1;
			break;
		}
	}
#ifdef DEBUG
	printk("key_isr() irq : %d, keyNumber : %d\n", irq, pKeyData->keyNumber);
#endif

	wake_up_interruptible(&WaitQueue_Read);

	return IRQ_HANDLED;
}

static int requestIrqInit(keyData *pData)
{
	int result = 0;
	char *irqName[8] = {"IrqKey0", "IrqKey1", "IrqKey2", "IrqKey3", "IrqKey4", "IrqKey5", "IrqKey6", "IrqKey7"};
	for(int i = 0; i < GPIOKEYCNT; i++)
	{
		result = request_irq(pData->key_irq[i], key_isr, IRQF_TRIGGER_RISING, irqName[i], pData);
		if(result < 0)
		{
			printk("request_irq() Failed irq %d\n", pData->key_irq[i]);
			return result;
		}
	}
	return result;
}

int	gpioLedInit(void)
{
	int i;
	int ret = 0;
	char gpioName[10];
	for(i=0;i<GPIOLEDCNT;i++)
	{

		sprintf(gpioName,"led%d",gpioLed[i]);
		ret = gpio_request(gpioLed[i], gpioName);
		if(ret < 0) {
			printk("Failed Request gpio%d error\n", 6);
			return ret;
		}
	}
	for(i=0;i<GPIOLEDCNT;i++)
	{
		ret = gpio_direction_output(gpioLed[i], OFF);
		if(ret < 0) {
			printk("Failed direction_output gpio%d error\n", 6);
       	 return ret;
		}
	}
	return ret;
}

void gpioLedSet(long val) 
{
	int i;
	for(i=0;i<GPIOLEDCNT;i++)
	{
		gpio_set_value(gpioLed[i], (val>>i) & 0x01);
	}
}
void gpioLedFree(void)
{
	int i;
	for(i=0;i<GPIOLEDCNT;i++)
	{
		gpio_free(gpioLed[i]);
	}
}
int gpioKeyInit(void) 
{
	int i;
	int ret=0;
	char gpioName[10];
	for(i=0;i<GPIOKEYCNT;i++)
	{
		sprintf(gpioName,"key%d",gpioKey[i]);
		ret = gpio_request(gpioKey[i], gpioName);
		if(ret < 0) {
			printk("Failed Request gpio%d error\n", 6);
			return ret;
		}
	}
	for(i=0;i<GPIOKEYCNT;i++)
	{
		ret = gpio_direction_input(gpioKey[i]);
		if(ret < 0) {
			printk("Failed direction_output gpio%d error\n", 6);
       	 return ret;
		}
	}
	return ret;
}
/*
int	gpioKeyGet(void) 
{
	int i;
	int ret;
	int keyData=0;
	for(i=0;i<GPIOKEYCNT;i++)
	{
		ret=gpio_get_value(gpioKey[i]) << i;
		keyData |= ret;
	}
	return keyData;
}
*/
void gpioKeyFree(void) 
{
	int i;
	for(i=0;i<GPIOKEYCNT;i++)
	{
		gpio_free(gpioKey[i]);
	}
}

int gpioKeyIrq(keyData *pData)
{
	int i;
	for(i=0;i<GPIOKEYCNT;i++)
	{
		pData->key_irq[i] = gpio_to_irq(gpioKey[i]);
		if(pData->key_irq[i] < 0)
		{
			printk("gpioKeyIrq() Failed gpio %d\n", gpioKey[i]);
			return pData->key_irq[i];
		}
	}
	return 0;
}

void gpioKeyIrqFree(keyData *pData)
{
	int i;
	for(i=0;i<GPIOKEYCNT;i++)
	{
		free_irq(pData->key_irq[i], pData);
	}
}

static int ledkey_open (struct inode *inode, struct file *filp)
{
	int result = 0;
    int num0 = MAJOR(inode->i_rdev); 
    int num1 = MINOR(inode->i_rdev); 
	keyData *pKeyData = (keyData *)kmalloc(sizeof(keyData), GFP_KERNEL);
	if(!pKeyData)
		return -ENOMEM;
	memset(pKeyData, 0, sizeof(keyData));

#ifdef DEBUG
    printk( "ledkey open -> major : %d\n", num0 );
    printk( "ledkey open -> minor : %d\n", num1 );
#endif

	result = gpioLedInit();
	if(result < 0)
		return result;
	result = gpioKeyInit();
	if(result < 0)
		return result;

	result = gpioKeyIrq(pKeyData);
	if(result < 0)
		return result;

	filp->private_data = pKeyData;

	result = requestIrqInit(pKeyData);
	if(result < 0)
		return result;

	try_module_get(THIS_MODULE);

    return 0;
}

static ssize_t ledkey_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	int ret;
	//char kbuf;
	keyData *pKeyData = (keyData *)filp->private_data;
	//kbuf = pKeyData->keyNumber;

	if(pKeyData->keyNumber == 0)
	{
		if(!(filp->f_flags & O_NONBLOCK))
		{
			wait_event_interruptible(WaitQueue_Read, pKeyData->keyNumber);
			//wait_event_interruptible_timeout(WaitQueue_Read, pKeyData->keyNumber, 100);  //100Hz : 100/100Hz = 1Sec
		}
	}
	//ret = copy_to_user(buf, &kbuf, count);
	ret = copy_to_user(buf, &(pKeyData->keyNumber), count);
	pKeyData->keyNumber = 0;
    return count;
}

static ssize_t ledkey_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	int ret;
	char kbuf;	
	ret = copy_from_user(&kbuf, buf, count);
	gpioLedSet(kbuf);
	ledVal = kbuf;
    return count;
}

static long ledkey_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	keyled_data ctrl_info = {0};
	int err=0, size;
	if(_IOC_TYPE(cmd) != IOCTLTEST_MAGIC) return -EINVAL;
	if(_IOC_NR(cmd) >= IOCTLTEST_MAXNR) return -EINVAL;

	size = _IOC_SIZE(cmd);
	if(size)
	{
		if(_IOC_DIR(cmd) & _IOC_READ)
		err = access_ok( (void *)arg, size);
		if(_IOC_DIR(cmd) & _IOC_WRITE)
			err = access_ok( (void *)arg, size);
		if(!err) return err;
	}

	switch(cmd)
	{
		case TIMER_START : 
			if(timer_pending(&timerLed))
			{
                del_timer(&timerLed);
			}
			kerneltimer_registertimer(timerVal);
			break;
		case TIMER_STOP :
			if(timer_pending(&timerLed))
        		del_timer(&timerLed);
			break;
		case TIMER_VALUE :
			err = copy_from_user((void *)&ctrl_info, (void *)arg, (unsigned long)sizeof(ctrl_info));
				timerVal = ctrl_info.timer_val;
			break;
		default :
			err = -E2BIG;
			break;
	}
	return err;
}

static unsigned int ledkey_poll(struct file * filp, struct poll_table_struct * wait)
{
	unsigned int mask=0;
	keyData * pKeyData = (keyData *)filp->private_data;
#ifdef DEBUG
	printk("_key : %u\n", (wait->_key & POLLIN));
#endif
	if(wait->_key & POLLIN)
		poll_wait(filp, &WaitQueue_Read, wait);
	if(pKeyData->keyNumber > 0)
		mask = POLLIN;

	return mask;
}

static int ledkey_release (struct inode *inode, struct file *filp)
{
	keyData *pKeyData = (keyData *)filp->private_data;
    printk( "ledkey release \n" );
	module_put(THIS_MODULE);
	gpioLedFree();
	gpioKeyIrqFree(pKeyData);
	gpioKeyFree();
	if(pKeyData) 
		kfree(pKeyData);
	del_timer(&timerLed);
    return 0;
}

struct file_operations ledkey_fops =
{
    .open     = ledkey_open,     
    .read     = ledkey_read,     
    .write    = ledkey_write,    
	.unlocked_ioctl = ledkey_ioctl,
	.poll     = ledkey_poll,
    .release  = ledkey_release,  
};

static int ledkey_init(void)
{
    int result;

    printk( "ledkey ledkey_init \n" );    

    result = register_chrdev( LEDKEY_MAJOR, LEDKEY_NAME, &ledkey_fops);
    if (result < 0) return result;


    return 0;
}

static void ledkey_exit(void)
{
    printk( "ledkey ledkey_exit \n" );    
    unregister_chrdev( LEDKEY_MAJOR, LEDKEY_NAME );
	gpioLedSet(0);
	del_timer(&timerLed);
	
}

module_init(ledkey_init);
module_exit(ledkey_exit);

MODULE_LICENSE("Dual BSD/GPL");
