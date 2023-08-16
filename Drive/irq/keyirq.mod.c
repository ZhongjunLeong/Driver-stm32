#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section(__versions) = {
	{ 0x4b3323eb, "module_layout" },
	{ 0x7e5a3c80, "device_destroy" },
	{ 0x97934ecf, "del_timer_sync" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0xc1514a3b, "free_irq" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0xff2f3447, "cdev_del" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x75736401, "class_destroy" },
	{ 0x2131a349, "device_create" },
	{ 0x9192c2a4, "__class_create" },
	{ 0xdd504b4a, "cdev_add" },
	{ 0x294045e0, "cdev_init" },
	{ 0xfe990052, "gpio_free" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x2072ee9b, "request_threaded_irq" },
	{ 0xd7be285f, "irq_get_irq_data" },
	{ 0x5a0629c5, "gpiod_direction_input" },
	{ 0x47229b5c, "gpio_request" },
	{ 0x3a2c7ce1, "irq_of_parse_and_map" },
	{ 0xceb52a7e, "of_get_named_gpio_flags" },
	{ 0xe2d5255a, "strcmp" },
	{ 0xf4d357f8, "of_property_read_string" },
	{ 0xc5850110, "printk" },
	{ 0x95b97b18, "of_find_node_opts_by_path" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x189c5980, "arm_copy_to_user" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0x526c3a6c, "jiffies" },
	{ 0x39a12ca7, "_raw_spin_unlock_irqrestore" },
	{ 0x9c3c5123, "gpiod_get_raw_value" },
	{ 0xc6e479b3, "gpio_to_desc" },
	{ 0x5f849a69, "_raw_spin_lock_irqsave" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
};

MODULE_INFO(depends, "");

