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

static struct usb_controller {
  struct usb_device *usb_dev;
  int pipe;
  dma_addr_t input_dma_addr;
  dma_addr_t output_dma_addr;
  char *buffer;
  struct urb *my_urb;
  struct urb *irq_out
  struct usb_anchor irq_out_anchor;
  char* output_data;
};

static void init_output(struct usb_interface *interface, struct usb_controller *controller, struct usb_endpoint_descriptor *ep_irq_out) {
  
  init_usb_anchor(&controller->irq_out_anchor);

  controller->output_data = usb_alloc_coherent(controller->usb_device, 64, GFP_KERNEL, controller->output_dma_addr);

  if (!controller->output_data) {
    printk(KERN_WARNING "ERROR: Could not usb_alloc_coherent.");
    return;
  }
  
  controller->irq_out = usb_alloc_urb(0, GFP_KERNEK);
  if (!controller->irq_out) {
    printk(KERN_WARNING "ERROR:Could not usb_alloc_urb");
    return;
  }

  usb_fill_int_urb(controller->irq_out, controller->usb_dev, usb_sndintpipe(controller->usb_dev, ep_irq_out->bEndpointAddress), controller->output_data, 64, output_callback, controller, ep_irq_out->bInterval);

  controller->irq_out->transfer_dma = controller->output_dma_addr;

}

static void read_callback(struct urb *urb) {

  printk(KERN_INFO "Within callback");

  struct usb_controller *controller = urb->context;
  char* data = controller->buffer;

  printk(KERN_ALERT "URB STATUS: %d", urb->status);

  printk(KERN_ALERT "data[0]=%d\n", data[0]);
	printk(KERN_ALERT "data[1]=%d\n", data[1]);
	printk(KERN_ALERT "data[2]=%d\n", data[2]);
	printk(KERN_ALERT "data[3]=%d\n", data[3]);
	printk(KERN_ALERT "data[4]=%d\n", data[4]);
	printk(KERN_ALERT "data[5]=%d\n", data[5]);
	printk(KERN_ALERT "data[6]=%d\n", data[6]);
	printk(KERN_ALERT "data[7]=%d\n", data[7]);
  
  int submit_val = usb_submit_urb(urb, GFP_ATOMIC);
  printk(KERN_INFO "submit_val: %d", submit_val);

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
    
    struct usb_controller *controller = kzalloc(sizeof(struct usb_controller) , GFP_KERNEL);
    if (!controller) {
      printk(KERN_WARNING "ERROR: Could not alloc controller.");
      return retval;
    }

    controller->my_urb = usb_alloc_urb(0, GFP_KERNEL);
    if (!controller->my_urb) {
      printk(KERN_WARNING "ERROR: Could not alloc urb.");
      return retval;
    }

    controller->usb_device = dev;
    
    struct usb_endpoint_descriptor *ep_irq_in, *ep_irq_out;
    ep_irq_in = NULL;
    ep_irq_out = NULL;

    for (int i = 0; i < 2; i++) {
      struct usb_endpoint_descriptor *ep = &interface->cur_altsetting->endpoint[i].desc;
      if (usb_endpoint_xfer_int(ep)) {
        if (usb_endpoint_dir_in(ep))
          ep_irq_in = ep;
        else
          ep_irq_out = ep;
      }
    }

    if (!ep_irq_in || !ep_irq_out) {
      printk(KERN_INFO "Muita treta\n");
      return retval;
    }

    controller->pipe = usb_rcvintpipe(dev, ep_irq_in->bEndpointAddress);
    controller->buffer = usb_alloc_coherent(dev, 64, GFP_KERNEL, &controller->input_dma_addr);
  
    init_output(interface, controller, ep_irq_out);

    printk(KERN_INFO "usb_fill_int_urb init\n");
    usb_fill_int_urb(controller->my_urb, dev, controller->pipe, controller->buffer, 64, read_callback, controller, ep_irq_in->bEndpointAddress);
 
    printk(KERN_INFO "usb_set_infdata init\n");
    usb_set_intfdata(interface, controller);
  
    printk(KERN_INFO "probe end");
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
    { USB_DEVICE(0x045e, 0x0b12) },
    {} // Entrada final
};
MODULE_DEVICE_TABLE (usb, minha_tabela_usb);

// USB - Definição do driver USB
static struct usb_driver meu_driver_usb =
{
    .name = "xbox",
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

