#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/usb/input.h>
#include <linux/hid.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lucas Nesi <email@email.com>");
MODULE_DESCRIPTION("Meu Modulo USB");

#define BUTTON_PRESSED 0x2C

#define A_BUTTON 0x10
#define B_BUTTON 0x20
#define X_BUTTON 0x40
#define Y_BUTTON 0x80

#define RB_BUTTON 0x20
#define LB_BUTTON 0x10

static struct usb_controller {
  struct usb_device *usb_dev;
  int pipe;
  dma_addr_t input_dma_addr;
  unsigned char *buffer;
  struct urb *my_urb;
};

static void get_button_pressed(const unsigned char* data) {
  if (data[3] == BUTTON_PRESSED) {
    unsigned char colored_buttons = data[4];
    if ((colored_buttons &  A_BUTTON) > 1)
      printk(KERN_INFO "BUTTON (A) pressed\n");
    if ((colored_buttons & B_BUTTON) > 1)
      printk(KERN_INFO "BUTTON (B) pressed\n");
    if ((colored_buttons & X_BUTTON) > 1)
      printk(KERN_INFO "BUTTON (X) pressed\n");
    if ((colored_buttons & Y_BUTTON) > 1)
      printk(KERN_INFO "BUTTON (Y) pressed\n");

    unsigned char top_buttons = data[5];
    printk(KERN_INFO "Operacao: %x", (top_buttons & LB_BUTTON) > 1);
    if ((top_buttons & RB_BUTTON) > 1)
      printk(KERN_INFO "BUTTON (RB) pressed");
    if ((top_buttons & LB_BUTTON) > 1)
      printk(KERN_INFO "BUTTON (LB) pressed");
  }
}

static void read_callback(struct urb *urb) {

  struct usb_controller *controller = urb->context;
  unsigned char* data = controller->buffer;

  printk(KERN_ALERT "data[0]=%X\n", data[0]);
	printk(KERN_ALERT "data[1]=%X\n", data[1]);
	printk(KERN_ALERT "data[2]=%X\n", data[2]);
	printk(KERN_ALERT "data[3]=%X\n", data[3]);
	printk(KERN_ALERT "data[4]=%X\n", data[4]);
	printk(KERN_ALERT "data[5]=%X\n", data[5]);
	printk(KERN_ALERT "data[6]=%X\n", data[6]);
	printk(KERN_ALERT "data[7]=%X\n\n", data[7]);

  get_button_pressed(data);

  int submit_val = usb_submit_urb(urb, GFP_ATOMIC);

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

    controller->usb_dev = dev;
    
    struct usb_endpoint_descriptor *ep_irq_in;
    ep_irq_in = NULL;

    for (int i = 0; i < 2; i++) {
      struct usb_endpoint_descriptor *ep = &interface->cur_altsetting->endpoint[i].desc;
      if (usb_endpoint_xfer_int(ep)) {
        if (usb_endpoint_dir_in(ep))
          ep_irq_in = ep;
      }
    }

    if (!ep_irq_in) {
      printk(KERN_WARNING "ERROR: Could not find interruption and in direction");
      return retval;
    }

    controller->pipe = usb_rcvintpipe(dev, ep_irq_in->bEndpointAddress);
    controller->buffer = usb_alloc_coherent(dev, 64, GFP_KERNEL, &controller->input_dma_addr);

    usb_fill_int_urb(controller->my_urb, dev, controller->pipe, controller->buffer, 64, read_callback, controller, ep_irq_in->bEndpointAddress);
 
    usb_set_intfdata(interface, controller);
 
    int submit_val = usb_submit_urb(controller->my_urb, GFP_ATOMIC);

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

