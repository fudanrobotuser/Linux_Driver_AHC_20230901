#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
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
__used __section("__versions") = {
	{ 0x4a4cf5e2, "module_layout" },
	{ 0xd6c8784c, "platform_driver_unregister" },
	{ 0x4552deae, "pnp_unregister_driver" },
	{ 0x2a13bb0c, "__platform_driver_register" },
	{ 0xf64ea225, "pnp_register_driver" },
	{ 0x239fe366, "close_candev" },
	{ 0xf3e9858b, "napi_disable" },
	{ 0x1416246, "alloc_can_skb" },
	{ 0x50296c1b, "can_get_echo_skb" },
	{ 0x7db0bb57, "can_bus_off" },
	{ 0x9d2ab8ac, "__tasklet_schedule" },
	{ 0xeecde651, "netif_device_attach" },
	{ 0xddd61cf6, "netif_tx_wake_queue" },
	{ 0xc4acb27e, "napi_enable" },
	{ 0xf87399c2, "open_candev" },
	{ 0xd61df3ab, "kfree_skb_reason" },
	{ 0x18d0f05f, "can_put_echo_skb" },
	{ 0xc1514a3b, "free_irq" },
	{ 0xeeef2ff3, "acpi_match_device" },
	{ 0x2850ef7f, "device_create_file" },
	{ 0x697fdf4, "register_candev" },
	{ 0x48163bba, "netif_napi_add" },
	{ 0xff3be4bc, "alloc_candev_mqs" },
	{ 0x4c42b4e7, "rt_mutex_base_init" },
	{ 0x2364c85a, "tasklet_init" },
	{ 0x34d1f098, "kmem_cache_alloc_trace" },
	{ 0x57f660fc, "kmalloc_caches" },
	{ 0x37a0cba, "kfree" },
	{ 0x7db8ff3b, "free_candev" },
	{ 0x609f1c7e, "synchronize_net" },
	{ 0x32405450, "__netif_napi_del" },
	{ 0x5b1783d6, "unregister_candev" },
	{ 0x3360f611, "device_remove_file" },
	{ 0xea3c74e, "tasklet_kill" },
	{ 0x65487097, "__x86_indirect_thunk_rax" },
	{ 0x2169fd9f, "mutex_unlock" },
	{ 0xbc186377, "mutex_lock" },
	{ 0x87e200a7, "netif_device_detach" },
	{ 0x8a5230f9, "netif_receive_skb" },
	{ 0xe1c50fe5, "alloc_can_err_skb" },
	{ 0xd47aa623, "can_free_echo_skb" },
	{ 0x92d5838e, "request_threaded_irq" },
	{ 0x22c1eb16, "rt_spin_unlock" },
	{ 0x442d8726, "rt_spin_lock" },
	{ 0xde80cd09, "ioremap" },
	{ 0x85bd1608, "__request_region" },
	{ 0x2e659147, "platform_get_resource" },
	{ 0xb69d2bf, "pnp_get_resource" },
	{ 0xdbdf6c92, "ioport_resource" },
	{ 0x1035c7c2, "__release_region" },
	{ 0x77358855, "iomem_resource" },
	{ 0xedc03953, "iounmap" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x92997ed8, "_printk" },
	{ 0xd0da656b, "__stack_chk_fail" },
	{ 0x9ec6ca96, "ktime_get_real_ts64" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "can-dev");

MODULE_ALIAS("acpi*:PNP1070:*");
MODULE_ALIAS("acpi*:AHC0512:*");
MODULE_ALIAS("of:N*T*Crdc,ahc-can");
MODULE_ALIAS("of:N*T*Crdc,ahc-canC*");
MODULE_ALIAS("platform:PNP1070");
MODULE_ALIAS("platform:AHC0512");
MODULE_ALIAS("pnp:dPNP1070*");
MODULE_ALIAS("acpi*:PNP1070:*");
MODULE_ALIAS("pnp:dAHC0512*");
MODULE_ALIAS("acpi*:AHC0512:*");

MODULE_INFO(srcversion, "E24E723254625FD13B06C91");
