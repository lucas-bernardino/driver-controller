#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/usb/input.h>
#include <linux/hid.h>

// Informações do Modulo
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lucas Nesi <email@email.com>");
MODULE_DESCRIPTION("Meu Modulo USB");

// **********************************
//   Interface com o subsistema USB
// **********************************

// Check what endpoints are not NULL
void check_endpoints(struct usb_device* dev, struct usb_interface *interface) {
    for (int i = 0; i < 16; i++) {
      if (dev->ep_in[i]) {
        printk(KERN_INFO "INTERRUPT STATE - dev->ep_in[%d]: %d", i, usb_endpoint_xfer_int(&dev->ep_in[i]->desc));
        printk(KERN_INFO "IN STATE - dev->ep_in[%d]: %d", i, usb_endpoint_dir_in(&dev->ep_in[i]->desc));
        // printk(KERN_INFO "ADDRESS - %04X", interface->cur_altsetting->endpoint[i].desc.bEndpointAddress);  
    }
    }
    for (int i = 0; i < 16; i++) {
      if (dev->ep_out[i]) {
        printk(KERN_INFO "INTERRUPT STATE - dev->ep_out[%d]: %d", i, usb_endpoint_xfer_int(&dev->ep_out[i]->desc));
        printk(KERN_INFO "IN STATE - dev->ep_out[%d]: %d", i, usb_endpoint_dir_in(&dev->ep_out[i]->desc));
      }
    }
}

void handle_urb(struct usb_device* dev, struct usb_interface *interface) {
  
  struct urb *my_urb = usb_alloc_urb(0, GFP_KERNEL);

  if (!my_urb) {
    printk(KERN_INFO "Error initialiazing urb");
  }
  
}


// USB - Probe - Função de entrada quando um novo dispositivo é reconhecido para este modulo
static int meu_driver_usb_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    int retval = 0;

    struct usb_device *dev = interface_to_usbdev(interface);

    if (interface->cur_altsetting->desc.bNumEndpoints != 2)
		return -ENODEV;

    printk(KERN_INFO "meu_driver_usb: O dispositivo idVendor=%X idProduct=%X foi connectado ao meu driver, interface=%X", id->idVendor, id->idProduct, interface->cur_altsetting->desc.bInterfaceNumber);

    int numendpoints = interface->cur_altsetting->desc.bNumEndpoints;
	
    printk(KERN_INFO "meu_driver_usb: interface=%X numEndpoints=%X", interface->cur_altsetting->desc.bInterfaceNumber, numendpoints);

    for (int i = 0; i < numendpoints; i++) {
      printk(KERN_INFO "ED[%d]->bEndpointAddress: 0x%02X\n", i, interface->cur_altsetting->endpoint[i].desc.bEndpointAddress);
      printk(KERN_INFO "ED[%d]->bmAttributes: 0x%02X\n", i, interface->cur_altsetting->endpoint[i].desc.bmAttributes);
      printk(KERN_INFO "ED[%d]->wMaxPacketSize: 0x%04X\n", i, interface->cur_altsetting->endpoint[i].desc.wMaxPacketSize);
    }

    check_endpoints(dev, interface);
    
    handle_urb(dev, interface);

    return retval;
}

// USB - Disconnect - Função de saída quando um dispositivo é desconectado
static void meu_driver_usb_disconnect(struct usb_interface *interface)
{
	printk(KERN_ALERT "Disconectando");
}

// USB - Tabela de dispositivos 
static struct usb_device_id minha_tabela_usb[] =
{
    { USB_DEVICE(0x045E, 0xB12) },
    {} // Entrada final
};
MODULE_DEVICE_TABLE (usb, minha_tabela_usb);

// USB - Definição do driver USB
static struct usb_driver meu_driver_usb =
{
    .name = "meu_driver_usb",
    .probe = meu_driver_usb_probe,
    .disconnect = meu_driver_usb_disconnect,
    .id_table = minha_tabela_usb,
};


// **********************
//   Controle do Módulo
// **********************

// Inicialização do modulo
static int __init meu_modulo_init(void)
{
    int result;

    // Registrando um dispositivo USB
    if ((result = usb_register(&meu_driver_usb)))
    {
        printk(KERN_ERR "Registro com erro: %d", result);
    }
    return result;
}

// Finalização do modulo
static void __exit meu_modulo_exit(void)
{
    // Desregistrando o dispositivo USB
    usb_deregister(&meu_driver_usb);
}

// Definição do modulo
// Entrypoint - A função que deve inicializar o modulo
module_init(meu_modulo_init);
// Exitpoint - A função que deve deinicializar o modulo
module_exit(meu_modulo_exit);

