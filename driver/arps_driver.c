#include <linux/kernel.h> 
#include <linux/module.h>
#include <linux/init.h> 
 
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>

#include <linux/version.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>

#include <linux/uaccess.h>

#include <linux/io.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>

#include <linux/mm.h>


#define ARPS_START_I 0
#define ARPS_READY_O 4

#define BUFF_SIZE 20

MODULE_AUTHOR("FTN");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("ARPS IP - Searches for Motion Vectors");
MODULE_ALIAS("custom:ARPS");

#define DRIVER_NAME "ARPS_DRIVER"
#define DEVICE_NAME "ARPS"


/*******************FUNCTION PROTOTYPES************************************/
static int arps_probe(struct platform_device *pdev);
static int arps_remove(struct platform_device *pdev);
static int arps_open(struct inode *pinode, struct file *pfile);
static int arps_close(struct inode *pinode, struct file *pfile);
static ssize_t arps_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset);
static ssize_t arps_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset);
static ssize_t arps_mmap(struct file *pfile, struct vm_area_struct *vma_s);



static int __init arps_init(void);
static void __exit arps_exit(void);

/*********************GLOBAL VARIABLES*************************************/
struct device_info {
  unsigned long mem_start;
  unsigned long mem_end;
  void __iomem *base_addr;

};


static dev_t my_dev_id;
static struct class *my_class;
static struct cdev *my_cdev;

//------------------------------------------
static struct device_info *arps      = NULL; //AXI_LITE
static struct device_info *bram_curr = NULL; //BRAM_CURR
static struct device_info *bram_ref  = NULL; //BRAM_REF
static struct device_info *bram_mv   = NULL; //BRAM_MV
//------------------------------------------

static struct of_device_id arps_of_match[] = {
    { .compatible = "xlnx,AXI-ARPS-IP"    , }, /*axi_lite_arps*/
    { .compatible = "xlnx,axi-bram-ctrl-0", }, /*bram_curr*/
    { .compatible = "xlnx,axi-bram-ctrl-1", }, /*bram_ref*/
    { .compatible = "xlnx,axi-bram-ctrl-2", }, /*bram_mv*/
    { /* end of list */ },
};

MODULE_DEVICE_TABLE(of, arps_of_match);


static struct platform_driver arps_driver = {
    .driver = {
    .name = DRIVER_NAME,
    .owner = THIS_MODULE,
    .of_match_table	= arps_of_match,
    },
    .probe = arps_probe,
    .remove = arps_remove,
};


struct file_operations my_fops =
{
    .owner = THIS_MODULE,
    .open = arps_open,
    .read = arps_read,
    .write = arps_write,
    .release = arps_close,
    .mmap = arps_mmap
};

int endRead = 0;
int device_fsm = 0;


// PROBE AND REMOVE
static int arps_probe(struct platform_device *pdev)
{
    struct resource *r_mem;
    int rc = 0;
    printk(KERN_INFO "Probing\n");

    r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!r_mem) {
        printk(KERN_ALERT "invalid address\n");
        return -ENODEV;
    }
    printk(KERN_INFO "Starting probing.\n");

    switch (device_fsm)
    {
    case 0: // device arps

        arps = (struct device_info *) kmalloc(sizeof(struct device_info), GFP_KERNEL);
        if (!arps)
        {
            printk(KERN_ALERT "Cound not allocate arps device\n");
            return -ENOMEM;
        }
        // Put phisical adresses in arps_info structure
        arps->mem_start = r_mem->start;
        arps->mem_end   = r_mem->end;

        // Reserve memory space for this driver
        if(!request_mem_region(arps->mem_start, arps->mem_end - arps->mem_start+1, DRIVER_NAME))
        {
            printk(KERN_ALERT "Couldn't lock memory region at %p\n",(void *)arps->mem_start);
            rc = -EBUSY;
            goto error1;
        }

        // Remap physical to virtual adresses
        arps->base_addr = ioremap(arps->mem_start, arps->mem_end - arps->mem_start + 1);
        if (!arps->base_addr)
        {
            printk(KERN_ALERT "[PROBE]: Could not allocate arps iomem\n");
            rc = -EIO;
            goto error2;
        }
        ++device_fsm;
        printk(KERN_INFO "[PROBE]: Finished probing arps.\n");
        return 0; //ALL OK
        error2:
            release_mem_region(arps->mem_start, arps->mem_end - arps->mem_start + 1);
        error1:
            return rc;
        break;

    case 1: // device bram_curr
        // Get memory for structure bram_curr_info
        bram_curr = (struct device_info *) kmalloc(sizeof(struct device_info), GFP_KERNEL);
        if(!bram_curr)
        {
            printk(KERN_ALERT "Cound not allocate bram_curr device\n");
            return -ENOMEM;
        }
        // Put phisical adresses in bram_curr_info structure
        bram_curr->mem_start = r_mem->start;
        bram_curr->mem_end   = r_mem->end;
         // Reserve that memory space for this driver
        if(!request_mem_region(bram_curr->mem_start, bram_curr->mem_end - bram_curr->mem_start+1, DRIVER_NAME))
        {
            printk(KERN_ALERT "Couldn't lock memory region at %p\n",(void *)bram_curr->mem_start);
            rc = -EBUSY;
            goto error3;
        }
        // Remap phisical to virtual adresses
        bram_curr->base_addr = ioremap(bram_curr->mem_start, bram_curr->mem_end - bram_curr->mem_start + 1);
        if (!bram_curr->base_addr)
        {
            printk(KERN_ALERT "[PROBE]: Could not allocate bram_curr iomem\n");
            rc = -EIO;
            goto error4;
        }
        ++device_fsm;
        printk(KERN_INFO "[PROBE]: Finished probing bram_curr.\n");
        return 0;//ALL OK
        error4:
            release_mem_region(bram_curr->mem_start, bram_curr->mem_end - bram_curr->mem_start + 1);
        error3:
            return rc;
        break;

    case 2: // device bram_ref
        bram_ref = (struct device_info *) kmalloc(sizeof(struct device_info), GFP_KERNEL);
        if (!bram_ref)
        {
            printk(KERN_ALERT "Cound not allocate bram_ref device\n");
            return -ENOMEM;
        }
        bram_ref->mem_start = r_mem->start;
        bram_ref->mem_end   = r_mem->end;
        if(!request_mem_region(bram_ref->mem_start, bram_ref->mem_end - bram_ref->mem_start+1, DRIVER_NAME))
        {
            printk(KERN_ALERT "Couldn't lock memory region at %p\n",(void *)bram_ref->mem_start);
            rc = -EBUSY;
            goto error5;
        }
        bram_ref->base_addr = ioremap(bram_ref->mem_start, bram_ref->mem_end - bram_ref->mem_start + 1);
        if (!bram_ref->base_addr)
        {
            printk(KERN_ALERT "[PROBE]: Could not allocate bram_ref iomem\n");
            rc = -EIO;
            goto error6;
        }
        ++device_fsm;
        printk(KERN_INFO "[PROBE]: Finished probing bram_ref.\n");
        return 0;
        error6:
            release_mem_region(bram_ref->mem_start, bram_ref->mem_end - bram_ref->mem_start + 1);
        error5:
            return rc;
        break;
    case 3: // device bram_mv
        bram_mv = (struct device_info *) kmalloc(sizeof(struct device_info), GFP_KERNEL);
        if (!bram_mv)
        {
            printk(KERN_ALERT "Cound not allocate bram_mv device\n");
            return -ENOMEM;
        }
        bram_mv->mem_start = r_mem->start;
        bram_mv->mem_end   = r_mem->end;
        if(!request_mem_region(bram_mv->mem_start, bram_mv->mem_end - bram_mv->mem_start+1, DRIVER_NAME))
        {
            printk(KERN_ALERT "Couldn't lock memory region at %p\n",(void *)bram_mv->mem_start);
            rc = -EBUSY;
            goto error7;
        }
        bram_mv->base_addr = ioremap(bram_mv->mem_start, bram_mv->mem_end - bram_mv->mem_start + 1);
        if (!bram_mv->base_addr)
        {
            printk(KERN_ALERT "[PROBE]: Could not allocate bram_mv iomem\n");
            rc = -EIO;
            goto error8;
        }
        printk(KERN_INFO "[PROBE]: Finished probing bram_mv.\n");
        return 0;
        error8:
            release_mem_region(bram_mv->mem_start, bram_mv->mem_end - bram_mv->mem_start + 1);
        error7:
            return rc;
        break;
    default:
        printk(KERN_INFO "[PROBE] Device FSM in illegal state. \n");
        return -1;
    }
    printk(KERN_INFO "Succesfully probed driver\n");
    return 0;
}
//---------------------------------------------------------------------------------------------------
static int arps_remove(struct platform_device *pdev)
{
    //free resources
    switch(device_fsm){
        case 0:{ //device arps
            printk(KERN_ALERT "arps device platform driver removed\n");
            iowrite32(0, arps->base_addr);
            iounmap(arps->base_addr);
            release_mem_region(arps->mem_start, arps->mem_end - arps->mem_start + 1);
            kfree(arps);
        }break;
        
        case 1:{ //device bram_curr
            printk(KERN_ALERT "bram_curr platform driver removed\n");
            iowrite32(0, bram_curr->base_addr);
            iounmap(bram_curr->base_addr);
            release_mem_region(bram_curr->mem_start, bram_curr->mem_end - bram_curr->mem_start + 1);
            kfree(bram_curr);
            --device_fsm;
        }break;

        case 2:{ //device bram_ref
            printk(KERN_ALERT "bram_ref platform driver removed\n");
            iowrite32(0, bram_ref->base_addr);
            iounmap(bram_ref->base_addr);
            release_mem_region(bram_ref->mem_start, bram_ref->mem_end - bram_ref->mem_start + 1);
            kfree(bram_ref);
            --device_fsm;
        }break;
	
        case 3:{ //device bram_mv
            printk(KERN_ALERT "bram_mv platform driver removed\n");
            iowrite32(0, bram_mv->base_addr);
            iounmap(bram_mv->base_addr);
            release_mem_region(bram_mv->mem_start, bram_mv->mem_end - bram_mv->mem_start + 1);
            kfree(bram_mv);
            --device_fsm;
        }break;
	
    default:
        printk(KERN_INFO "[REMOVE] Device FSM in illegal state. \n");
        return -1;
    }
    printk(KERN_INFO "Succesfully removed driver\n");
    return 0;
}
//---------------------------------------------------------------------------------------------------
// IMPLEMENTATION OF FILE OPERATION FUNCTIONS
static int arps_open(struct inode *pinode, struct file *pfile) 
{
    printk(KERN_INFO "Succesfully opened file\n");
    return 0;
}
//---------------------------------------------------------------------------------------------------
static int arps_close(struct inode *pinode, struct file *pfile) 
{
    printk(KERN_INFO "Succesfully closed file\n");
    return 0;
}
//---------------------------------------------------------------------------------------------------
static ssize_t arps_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset) 
{

    int ret;
    int len = 0;
    u32 start_i = 0;
    u32 ready_o = 0;

    int minor = MINOR(pfile->f_inode->i_rdev);
    char buff[BUFF_SIZE];
    if (endRead){
        endRead = 0;
        return 0;
    }

    switch(minor){
        case 0:{
            start_i = ioread32((arps->base_addr)+ARPS_START_I); //start
            ready_o = ioread32((arps->base_addr)+ARPS_READY_O); //ready
            //format => S: value; R: value;
            buff[0]='S';
            buff[1]=':';
            buff[2]=' ';
            if(start_i==0 && ready_o==0){
                buff[3]='0';//start
                buff[4]=';';
                buff[5]=' ';
                buff[6]='R';
                buff[7]=':';
                buff[8]=' ';
                buff[9]='0';//ready
                buff[10]='\n';
            }
            else if(start_i==0 && ready_o==1){
                buff[3]='0';//start
                buff[4]=';';
                buff[5]=' ';
                buff[6]='R';
                buff[7]=':';
                buff[8]=' ';
                buff[9]='1';//ready
                buff[10]='\n';
            }
            else if(start_i==1 && ready_o==0){
                buff[3]='1';//start
                buff[4]=';';
                buff[5]=' ';
                buff[6]='R';
                buff[7]=':';
                buff[8]=' ';
                buff[9]='0';//ready
                buff[10]='\n';
            }
            else if(start_i==1 && ready_o==1){
                buff[3]='1';//start
                buff[4]=';';
                buff[5]=' ';
                buff[6]='R';
                buff[7]=':';
                buff[8]=' ';
                buff[9]='1';//ready
                buff[10]='\n';
            }
            else{
                printk(KERN_ERR "Wrong reading!\n");
            }
            len=11;
            ret = copy_to_user(buffer, buff, len);
            if(ret)
                return -EFAULT;
            printk(KERN_INFO "[READ] Succesfully read from VMD device.\n");
            endRead = 1;
        }break;
        
        case 1:{
            printk(KERN_INFO "[READ] Can't read from BRAM_CURR, use MMAP.\n");
            endRead = 1;
        }break;
		
        case 2:{
            printk(KERN_INFO "[READ] Can't read from BRAM_REF, use MMAP.\n");
            endRead = 1;
        }break;
		
        case 3:{
            printk(KERN_INFO "[READ] Can't read from BRAM_MV, use MMAP.\n");
            endRead = 1;
        }break;
		
        default:
            printk(KERN_ERR "[READ] Invalid minor. \n");
            endRead = 1;
    }
    return len;
}
//---------------------------------------------------------------------------------------------------
static ssize_t arps_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset) 
{

    char buff[BUFF_SIZE];
    int ret = 0;

    int minor = MINOR(pfile->f_inode->i_rdev);
    //copying from user space
    ret = copy_from_user(buff, buffer, length);//buffer=>buff
    if(ret){
        printk("copy from user failed \n");
        return -EFAULT;
    }  
    buff[length] = '\0';
  
    switch(minor){
	  
        case 0:{ //device arps
            if( *(buff)  =='s' && 
                *(buff+1)=='t' &&
                *(buff+2)=='a' &&
                *(buff+3)=='r' &&
                *(buff+4)=='t' &&
                *(buff+5)==' ' &&
                *(buff+6)=='=' &&
                *(buff+7)==' '){
                    
                if(*(buff+8)=='1' && length==10){
                    iowrite32(0x00000001,(arps->base_addr)+ARPS_START_I);//start=1;
                    printk(KERN_INFO "[WRITE] Succesfully write VMD: start = 1\n");

                }
                else if(*(buff+8)=='0' && length==10){
                    iowrite32(0x00000000,(arps->base_addr)+ARPS_START_I);//start=0
                    printk(KERN_INFO "[WRITE] Succesfully write VMD: start = 0\n");
                }
                else{
                    printk(KERN_ERR "[WRITE] Invalid format for writing in VMD!\n");
                }
            }
            else{
                printk(KERN_ERR "[WRITE] Invalid format for writing in VMD!\n");
            }
        }break;
        
        case 1:{//device bram_curr
            printk(KERN_INFO "[WRITE] Can't write to BRAM_CURR, use MMAP. \n");  

        }break;
        
        case 2:{//device bram_ref
            printk(KERN_INFO "[WRITE] Can't write to BRAM_REF, use MMAP. \n");  

        }break;
	  
        case 3:{//device bram_mv
            printk(KERN_INFO "[WRITE] Can't write to BRAM_MV, use MMAP. \n");  
        }break;
	  
        default:
            printk(KERN_INFO "[WRITE] Invalid minor. \n");
    }
    
    return length;
}
static ssize_t arps_mmap(struct file *pfile, struct vm_area_struct *vma_s){
    int ret = 0;
    int minor = MINOR(pfile->f_inode->i_rdev);
    unsigned long vir_addr_size;
    unsigned long ph_addr_size;
 	
    switch(minor){
        case 0:{// AXI LITE
            printk(KERN_INFO "MMAP is not implemented for VMD device\n");
        }break;

        case 1:{//bram_ctrl 0
            vir_addr_size=vma_s->vm_end - vma_s->vm_start;
            ph_addr_size = (bram_curr->mem_end - bram_curr->mem_start)+1;
            //printk(KERN_INFO "BRAM_CURR: vir_addr_size=%lu , ph _addr_size=%lu \n",vir_addr_size,ph_addr_size);
            /*
                https://www.oreilly.com/library/view/linux-device-drivers/0596000081/ch13s02.html
            */
            vma_s->vm_page_prot = pgprot_noncached(vma_s-> vm_page_prot);//disable caching
            printk(KERN_INFO "BRAM_CURR: Buffer is being memory mapped\n");			
            if (vir_addr_size > ph_addr_size){
                printk(KERN_ERR "BRAM_CURR: Trying to mmap more space than it's allocated, mmap failed\n");
                return -EIO;
            }
            ret = vm_iomap_memory(vma_s,bram_curr->mem_start, vir_addr_size);//map virtual and phisical addresses
            if(ret){
                printk(KERN_ERR "BRAM_CURR: memory maped failed\n");
                return ret;
            }
            printk(KERN_INFO "MMAP is a success for BRAM_CURR\n");
			
            }break;
            
        case 2:{
            vir_addr_size=vma_s->vm_end - vma_s->vm_start;
            ph_addr_size = (bram_ref->mem_end - bram_ref->mem_start)+1;
            //printk(KERN_INFO "BRAM_REF: vir_addr_size=%lu , ph_addr_size=%lu \n",vir_addr_size,ph_addr_size);
            vma_s->vm_page_prot = pgprot_noncached(vma_s-> vm_page_prot);
            printk(KERN_INFO "BRAM_REF: Buffer is being memory mapped\n");
            if (vir_addr_size > ph_addr_size){
                printk(KERN_ERR "BRAM_REF: Trying to mmap more space than it's allocated, mmap failed\n");
                return -EIO;
            }
            ret = vm_iomap_memory(vma_s, bram_ref->mem_start, vir_addr_size);
            if(ret){
                printk(KERN_ERR "BRAM_REF: memory maped failed\n");
                return ret;
            }
            printk(KERN_INFO "MMAP is a success for BRAM_REF\n");
        }break;
		
        case 3:{
            vir_addr_size=vma_s->vm_end - vma_s->vm_start;
            ph_addr_size = (bram_mv->mem_end - bram_mv->mem_start)+1;
            //printk(KERN_INFO "BRAM_MV: vir_addr_size=%lu, ph_addr_size=%lu\n",vir_addr_size,ph_addr_size);
            vma_s->vm_page_prot = pgprot_noncached(vma_s-> vm_page_prot);
            printk(KERN_INFO "BRAM_MV: Buffer is being memory mapped\n");
            if (vir_addr_size > ph_addr_size){
                printk(KERN_ERR "BRAM_MV: Trying to mmap more space than it's allocated, mmap failed\n");
                return -EIO;
            }
            ret = vm_iomap_memory(vma_s, bram_mv->mem_start, vir_addr_size);
            if(ret){
                printk(KERN_ERR "BRAM_MV: memory maped failed\n");
                return ret;
            }
            printk(KERN_INFO "MMAP is a success for BRAM_MV\n");
        }break;
		
        default:
            printk(KERN_INFO "[WRITE] Invalid minor. \n");
    }
	
    return 0;

}
// INIT AND EXIT FUNCTIONS OF THE DRIVER
static int __init arps_init(void)
{

    //Same MAJOR different MINOR number
    int num_of_minors = 4;
    printk(KERN_INFO "\n");
    printk(KERN_INFO "arps_init: Initialize Module \"%s\"\n", DEVICE_NAME);
    
    //allocate 4 MINOR numbers

    if (alloc_chrdev_region(&my_dev_id, 0, num_of_minors, "arps_region") < 0){
        printk(KERN_ERR "failed to register char device\n");
        return -1;
    }
    printk(KERN_INFO "char device region allocated\n");

    my_class = class_create(THIS_MODULE, "arps_class");

    if (my_class == NULL){
        printk(KERN_ERR "failed to create class\n");
        goto fail_0;
    }
    printk(KERN_INFO "class created\n");

    if (device_create(my_class, NULL, MKDEV(MAJOR(my_dev_id),0), NULL, "vmd") == NULL) {
        printk(KERN_ERR "failed to create device vmd\n");
        goto fail_1;
    }
    printk(KERN_INFO "device created - vmd\n");

    if (device_create(my_class, NULL, MKDEV(MAJOR(my_dev_id),1), NULL, "br_ctrl_0") == NULL) {
        printk(KERN_ERR "failed to create device bram_curr\n");
        goto fail_2;
    }
    printk(KERN_INFO "device created - bram_curr\n");

    if (device_create(my_class, NULL, MKDEV(MAJOR(my_dev_id),2), NULL, "br_ctrl_1") == NULL) {
        printk(KERN_ERR "failed to create device bram_ref\n");
        goto fail_3;
    }
    printk(KERN_INFO "device created - bram_mv\n");
   
    if (device_create(my_class, NULL, MKDEV(MAJOR(my_dev_id),3), NULL, "br_ctrl_2") == NULL) {
        printk(KERN_ERR "failed to create device bram_mv\n");
        goto fail_4;
    }
    printk(KERN_INFO "device created - bram_mv\n");

    my_cdev = cdev_alloc();
    my_cdev->ops = &my_fops;
    my_cdev->owner = THIS_MODULE;
    

    if (cdev_add(my_cdev, my_dev_id, num_of_minors) == -1){
        printk(KERN_ERR "failed to add cdev\n");
        goto fail_5;
    }
    printk(KERN_INFO "cdev added\n");
    printk(KERN_INFO "ARPS driver initialized.\n");

    return platform_driver_register(&arps_driver);

    fail_5:
        device_destroy(my_class, MKDEV(MAJOR(my_dev_id),3));
    fail_4:
        device_destroy(my_class, MKDEV(MAJOR(my_dev_id),2));
    fail_3:
        device_destroy(my_class, MKDEV(MAJOR(my_dev_id),1));
    fail_2:
        device_destroy(my_class, MKDEV(MAJOR(my_dev_id),0));
    fail_1:
        class_destroy(my_class);
    fail_0:
        unregister_chrdev_region(my_dev_id, 1);
    return -1;
}

static void __exit arps_exit(void)
{
    printk(KERN_INFO "ARPS driver starting rmmod.\n");
    platform_driver_unregister(&arps_driver);
    cdev_del(my_cdev);

    device_destroy(my_class, MKDEV(MAJOR(my_dev_id),3));
    device_destroy(my_class, MKDEV(MAJOR(my_dev_id),2));
    device_destroy(my_class, MKDEV(MAJOR(my_dev_id),1));
    device_destroy(my_class, MKDEV(MAJOR(my_dev_id),0));
    class_destroy(my_class);
    unregister_chrdev_region(my_dev_id,1);
    printk(KERN_INFO "arps_exit: Exit Device Module \"%s\".\n", DEVICE_NAME);
}

module_init(arps_init);
module_exit(arps_exit);
