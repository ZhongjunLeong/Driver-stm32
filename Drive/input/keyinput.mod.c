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
	{ 0x9db1c0ee, "platform_driver_unregister" },
	{ 0x37bb128, "__platform_driver_register" },
	{ 0xc5850110, "printk" },
	{ 0x24d2061c, "input_register_device" },
	{ 0x4b6ac138, "input_set_capability" },
	{ 0xec602449, "input_allocate_device" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x2072ee9b, "request_threaded_irq" },
	{ 0xd7be285f, "irq_get_irq_data" },
	{ 0x3a2c7ce1, "irq_of_parse_and_map" },
	{ 0x5a0629c5, "gpiod_direction_input" },
	{ 0x47229b5c, "gpio_request" },
	{ 0xceb52a7e, "of_get_named_gpio_flags" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0x526c3a6c, "jiffies" },
	{ 0x27bbf221, "disable_irq_nosync" },
	{ 0xfcec0987, "enable_irq" },
	{ 0x5de00dd, "input_event" },
	{ 0x9c3c5123, "gpiod_get_raw_value" },
	{ 0xc6e479b3, "gpio_to_desc" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x26a6195e, "input_unregister_device" },
	{ 0x97934ecf, "del_timer_sync" },
	{ 0xfe990052, "gpio_free" },
	{ 0xc1514a3b, "free_irq" },
};

MODULE_INFO(depends, "");

