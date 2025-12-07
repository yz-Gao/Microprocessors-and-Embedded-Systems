#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
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
__used
__attribute__((section("__versions"))) = {
	{ 0xd10c774, "module_layout" },
	{ 0x915341b4, "kthread_stop" },
	{ 0xe7043306, "iounmap" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0x509980a0, "wake_up_process" },
	{ 0xde7f8e66, "kthread_create_on_node" },
	{ 0xd5e9b1dc, "__register_chrdev" },
	{ 0xcdbf1cc, "ioremap" },
	{ 0xfb578fc5, "memset" },
	{ 0xf9a482f9, "msleep" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0xad27f361, "__warn_printk" },
	{ 0xe526f136, "__copy_user" },
	{ 0x28318305, "snprintf" },
	{ 0x7c32d0f0, "printk" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "80FA2BE7E9A1E7887DDB531");
