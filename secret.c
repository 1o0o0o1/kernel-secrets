#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/random.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/string.h>

#define SUCCESS 0

// A struct for storing secrets
struct secret {
    char *data;
    size_t size;
};

enum secret_operation_type {
    SECRET_SET_INDEX,
    SECRET_READ,
    SECRET_WRITE,
    SECRET_DELETE
};

#define SECRET_MAX_INDEX 1023

// the number of index digits entered
#define SECRET_MAX_INPUT_SIZE 4

struct secret_table {
    struct secret *secret[SECRET_MAX_INDEX];
};

static unsigned int _index = 0;
static struct secret_table my_secret_table;

void init_secret_table(struct secret_table *table) {
    for (int i = 0; i < SECRET_MAX_INDEX + 1; i++) {
        table->secret[i] = NULL; 
    }
}

void free_secret_table(struct secret_table *table) {
    for (int i = 0; i < SECRET_MAX_INDEX + 1; i++) {
        if (table->secret[i] != NULL) {
            vfree(table->secret[i]->data); 
            kfree(table->secret[i]); 
        }
    }
}

static ssize_t check_input(char *input, const char *buf, size_t len,
                        enum secret_operation_type op_type)
{
    /*
    * This function accepts *input and, if it correct, sets it by copying data from *buf. 
    * 
    * The op_type argument defines the type of check and sets
    * criteria for a specific type of operation.
    * For example, in the case of SECRET_SET_INDEX,
    * it is checked that the input does not exceed SECRET_MAX_INPUT_SIZE 
    *
    */

    if(len == 0){
        pr_err("Error: data cannot be empty\n");
        return -ENODATA;
    }

    if (copy_from_user(input, buf, len)) {
        pr_err("Error: Copy form userspace to kernelspace is failed\n");
        return -EFAULT;
    }
    
    if((op_type == SECRET_SET_INDEX)){
        if(len > SECRET_MAX_INPUT_SIZE){
            pr_err("Error: Long input\n");
            return -EINVAL;
        }
        if(kstrtouint(input, 10, &_index)) {
            pr_err("Error: Cannot convert user input to digit. Input must be a positive number\n");
            return -EINVAL;
        }
    }

    return SUCCESS;
}

static ssize_t check_index(unsigned int index, enum secret_operation_type op_type)
{

    /*
    * This function checks the index and access to data for this index. 
    */

    if (index > SECRET_MAX_INDEX) {
        pr_err("Error: index is out of range 0 to %d!\n", SECRET_MAX_INDEX);
        return -EINVAL;
    }

    if(op_type == SECRET_READ || op_type == SECRET_DELETE){
        if(my_secret_table.secret[index] == NULL){
            pr_err("Error: No data found with index=%d!\n", index);
            return -ENODATA;
        }
    }

    if(op_type == SECRET_WRITE){
        if(my_secret_table.secret[index] != NULL){
            pr_err("Secret is already stored under the index %d!\n", index);
            return -EFAULT;
        }
    }

    return SUCCESS;
}

static ssize_t secret_set_index(struct file *file, const char __user *buf, 
                                size_t len, loff_t *off)
{
    ssize_t ret;
    char input_buffer[SECRET_MAX_INPUT_SIZE];
    if((ret = check_input(input_buffer, buf, len - 1, SECRET_SET_INDEX)))
        return ret;

    if(ret = check_index(_index, SECRET_SET_INDEX))
        return ret;

    pr_info("Index set to %d.\n", _index);
    ret = len;
    return len;
}

static ssize_t secret_read(struct file *file, char __user *buf, 
                            size_t len, loff_t *off)
{
    ssize_t ret = 0;
    if(ret = check_index(_index, SECRET_READ))
        return ret;

    pr_info("Secret with index %d: %s\n", _index, my_secret_table.secret[_index]->data);
    return ret;
}

static ssize_t secret_write(struct file *file, const char __user *buf, 
                            size_t len, loff_t *off)
{
    ssize_t ret;
    if(ret = check_index(_index, SECRET_WRITE))
        return ret;

    struct secret *secret = kmalloc(sizeof(struct secret), GFP_KERNEL);
    if(!secret){
        pr_err("Error: cannot allocate memory for secret\n");
        kfree(secret);
        return -ENOMEM;
    }
    secret->data = vmalloc(len);
    if(!secret->data) {
        vfree(secret->data);
        kfree(secret);

        pr_err("Error: cannot allocate memory for secret data\n");
        return -ENOMEM;
    }
    if(copy_from_user(secret->data, buf, len)) {
        vfree(secret->data);
        kfree(secret);

        pr_err("Error: copy form userspace to kernelspace is failed\n");
        return -EFAULT;
    }
    
    secret->size = len;
    secret->data[len] = '\0';

    my_secret_table.secret[_index] = secret;
    pr_info("Successful writing secret whith index %d\n", _index);

    return len;
}
static ssize_t secret_delete(struct file *file, char __user *buf, 
                              size_t len, loff_t *off)
{
    ssize_t ret = 0;
    if(ret = check_index(_index, SECRET_DELETE))
       return ret;

    vfree(my_secret_table.secret[_index]->data);
    kfree(my_secret_table.secret[_index]);
    my_secret_table.secret[_index] = NULL;

    pr_info("Secret with index %d success deleted\n", _index);
    return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
#define HAVE_PROC_OPS
#endif

#ifdef HAVE_PROC_OPS
static const struct proc_ops secret_setter_fops = {
    .proc_write  = secret_set_index,
};
static const struct proc_ops secret_read_fops = {
    .proc_read  = secret_read,
};
static const struct proc_ops secret_write_fops = {
    .proc_write  = secret_write,
};
static const struct proc_ops secret_delete_fops = {
    .proc_read = secret_delete,
};
#else
static const struct file_operations setter_fops = {
    .write  = secret_set_index,
};
static const struct file_operations secret_read_fops = {
    .read  = secret_read,
};
static const struct file_operations secret_write_fops = {
    .write  = secret_write,
};
static const struct file_operations secret_delete_fops = {
    .read  = secret_delete,
};
#endif

#define PROCFS_DIR "secret"
#define PROCFS_SECRET_SETTER_FILE "set_index"
#define PROCFS_SECRET_READ_FILE "read"
#define PROCFS_SECRET_WRITE_FILE "write"
#define PROCFS_SECRET_DELETE_FILE "delete"

static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *setter_file;
static struct proc_dir_entry *read_file;
static struct proc_dir_entry *write_file;
static struct proc_dir_entry *delete_file;

static int __init procfs2_init(void)
{
    init_secret_table(&my_secret_table);

    proc_dir    = proc_mkdir(PROCFS_DIR, NULL);
    setter_file = proc_create(PROCFS_SECRET_SETTER_FILE, 0110, proc_dir, &secret_setter_fops);
    read_file   = proc_create(PROCFS_SECRET_READ_FILE, 0220, proc_dir, &secret_read_fops);
    write_file  = proc_create(PROCFS_SECRET_WRITE_FILE, 0110, proc_dir, &secret_write_fops);
    delete_file = proc_create(PROCFS_SECRET_DELETE_FILE, 0220, proc_dir, &secret_delete_fops);

    if ((proc_dir == NULL) || (setter_file == NULL) || (read_file == NULL) || (write_file == NULL) || (delete_file == NULL)) {
        if(setter_file == NULL) {
            proc_remove(setter_file);
            pr_err("Error: Could not initialize /proc/%s/%s\n", PROCFS_DIR, PROCFS_SECRET_SETTER_FILE);
        }
        if(read_file == NULL) {
            proc_remove(read_file);
            pr_err("Error: Could not initialize /proc/%s/%s\n", PROCFS_DIR, PROCFS_SECRET_READ_FILE);
        }
        if(write_file == NULL) {
            proc_remove(write_file);
            pr_err("Error: Could not initialize /proc/%s/%s\n", PROCFS_DIR, PROCFS_SECRET_WRITE_FILE);
        }
        if(delete_file == NULL) {
            proc_remove(delete_file);
            pr_err("Error: Could not initialize /proc/%s/%s\n", PROCFS_DIR, PROCFS_SECRET_DELETE_FILE);
        }
        if (proc_dir == NULL) {
            proc_remove(proc_dir);
            pr_err("Error: Could not initialize /proc/%s\n", PROCFS_DIR);
        }
        return -ENOENT;
    }

    pr_info("Directory /proc/%s created\n", PROCFS_DIR);
    pr_info("File /proc/secret/%s created\n", PROCFS_SECRET_SETTER_FILE);
    pr_info("Index set by default %d", _index);
    pr_info("File /proc/secret/%s created\n", PROCFS_SECRET_READ_FILE);
    pr_info("File /proc/secret/%s created\n", PROCFS_SECRET_WRITE_FILE);
    pr_info("File /proc/secret/%s created\n", PROCFS_SECRET_READ_FILE);
    return SUCCESS;
}

static void __exit procfs2_exit(void)
{
    free_secret_table(&my_secret_table);

    proc_remove(setter_file);
    pr_info("/proc/%s/%s removed\n", PROCFS_DIR, PROCFS_SECRET_SETTER_FILE);

    proc_remove(read_file);
    pr_info("/proc/%s/%s removed\n", PROCFS_DIR, PROCFS_SECRET_READ_FILE);

    proc_remove(write_file);
    pr_info("/proc/%s/%s removed\n", PROCFS_DIR, PROCFS_SECRET_WRITE_FILE);

    proc_remove(delete_file);
    pr_info("/proc/%s/%s removed\n", PROCFS_DIR, PROCFS_SECRET_DELETE_FILE);

    proc_remove(proc_dir);
    pr_info("/proc/%s removed\n", PROCFS_DIR);
}

module_init(procfs2_init);
module_exit(procfs2_exit);

MODULE_LICENSE("GPL");
