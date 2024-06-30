#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/usb/input.h>
#include <linux/hid.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lucas Bernardino, Gustavo Moro, Pedro Augusto Dantas Vargas");
MODULE_DESCRIPTION("Xbox Series S Controller Driver");

#define BUTTON_PRESSED 0x2C

#define A_BUTTON 0x10
#define B_BUTTON 0x20
#define X_BUTTON 0x40
#define Y_BUTTON 0x80

#define RB_BUTTON 0x20
#define LB_BUTTON 0x10

#define UP_BUTTON 0x1 
#define BOTTOM_BUTTON 0x2 
#define LEFT_BUTTON 0x4
#define RIGHT_BUTTON 0x8

static struct usb_controller {
  struct usb_device *usb_dev;
  int pipe;
  dma_addr_t input_dma_addr;
  unsigned char *buffer;
  struct urb *my_urb;
  struct input_dev *i_dev;
  char *name;
};

static void handle_button_pressed(const unsigned char* data, struct usb_controller * controller) {
  if(data[3]  == BUTTON_PRESSED){
    unsigned char colored_buttons = data[4];
    unsigned char aux_buttons = data[5];
    input_report_key(controller->i_dev, KEY_SPACE, colored_buttons & A_BUTTON);
    input_report_key(controller->i_dev, KEY_B, colored_buttons & B_BUTTON);
    input_report_key(controller->i_dev, KEY_X, colored_buttons & X_BUTTON);
    input_report_key(controller->i_dev, KEY_Y, colored_buttons & Y_BUTTON);
    input_report_key(controller->i_dev, KEY_VOLUMEUP, aux_buttons & RB_BUTTON);
    input_report_key(controller->i_dev, KEY_VOLUMEDOWN, aux_buttons & LB_BUTTON);
    input_report_key(controller->i_dev, KEY_UP, aux_buttons & UP_BUTTON);
    input_report_key(controller->i_dev, KEY_DOWN, aux_buttons & BOTTOM_BUTTON);
    input_report_key(controller->i_dev, KEY_LEFT, aux_buttons & LEFT_BUTTON);
    input_report_key(controller->i_dev, KEY_RIGHT, aux_buttons & RIGHT_BUTTON);
  }
  input_sync(controller->i_dev);
}

static void read_callback(struct urb *urb) {

  struct usb_controller *controller = urb->context;
  unsigned char* data = controller->buffer;

  handle_button_pressed(data, controller);

  if (!controller->i_dev) {
	  printk(KERN_WARNING "controller->i_dev is null in read_callback\n");
	  return;
  }

  int submit_val = usb_submit_urb(urb, GFP_ATOMIC);
}

static int usb_controller_open(struct input_dev *i_dev) {

  struct usb_controller *controller = input_get_drvdata(i_dev);

  if (!controller) {
    printk(KERN_WARNING "ERROR: Could not input_get_drvdata\n");
    return 1;
  }

  printk(KERN_ALERT "OPENING USB_CONTROLLER\n");

  if (!controller->my_urb) {
    printk(KERN_WARNING "Opps.. controller->my_urb is NULL\n");
    return 1;
  }

  if (!controller->usb_dev) {
    printk(KERN_WARNING "Opps.. controller->usb_dev is NULL\n");
    return 1;
  }

  controller->my_urb->dev = controller->usb_dev;

  int status = usb_submit_urb(controller->my_urb, GFP_KERNEL);

  if (status) {
    printk(KERN_WARNING "ERROR: Could not usb_submit_urb\n");
    return 1;
  }

  return 0;

}

static void usb_controller_close(struct input_dev *i_dev) {

  struct usb_controller *controller = input_get_drvdata(i_dev);

  if (!controller) {
    printk(KERN_WARNING "ERROR: Could not input_get_drvdata\n");
    return;
  }

  if (!controller->my_urb) {
    printk(KERN_WARNING "Opps.. controller->my_urb is NULL\n");
    return;
  }

  printk(KERN_ALERT "CLOSING USB_CONTROLLER\n");

  usb_kill_urb(controller->my_urb);

}

// USB - Probe - Função de entrada quando um novo dispositivo é reconhecido para este modulo
static int meu_driver_usb_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
  int retval = 0;

  struct usb_device *dev = interface_to_usbdev(interface);

  if (interface->cur_altsetting->desc.bNumEndpoints != 2)
    return -ENODEV;

  printk(KERN_INFO "controller_driver: O dispositivo idVendor=%X idProduct=%X foi connectado ao meu driver, interface=%X", id->idVendor, id->idProduct, interface->cur_altsetting->desc.bInterfaceNumber);

  int numendpoints = interface->cur_altsetting->desc.bNumEndpoints;

  printk(KERN_INFO "controller_driver: interface=%X numEndpoints=%X", interface->cur_altsetting->desc.bInterfaceNumber, numendpoints);

  struct usb_controller *controller = kzalloc(sizeof(struct usb_controller) , GFP_KERNEL);
  if (!controller) {
    printk(KERN_WARNING "ERROR: Could not alloc controller.\n");
    return retval;
  }

  controller->my_urb = usb_alloc_urb(0, GFP_KERNEL);
  if (!controller->my_urb) {
    printk(KERN_WARNING "ERROR: Could not alloc urb.\n");
    return retval;
  }

  controller->name = "Xbox Series S Controller";
  controller->usb_dev = dev;

  struct usb_endpoint_descriptor *ep_irq_in;
  ep_irq_in = NULL;

  for (int i = 0; i < 2; i++) {
    struct usb_endpoint_descriptor *ep = &interface->cur_altsetting->endpoint[i].desc;
    if (usb_endpoint_xfer_int(ep)) {
      if (usb_endpoint_dir_in(ep)) {
        printk(KERN_INFO "controller_driver: Using [%x] as the bEndpointAddress", ep->bEndpointAddress);
        ep_irq_in = ep;
      }
    }
  }

  if (!ep_irq_in) {
    printk(KERN_WARNING "ERROR: Could not find interruption and in direction\n");
    return retval;
  }

  struct input_dev *i_dev = input_allocate_device();
  if(!i_dev) {
    printk(KERN_WARNING "ERROR: Could not input_allocate_device\n");
    return retval;
  }
  controller->i_dev = i_dev;

  usb_to_input_id(controller->usb_dev, &i_dev->id);
  i_dev->dev.parent = &interface->dev;
  controller->i_dev->name = controller->name;
  input_set_drvdata(i_dev, controller);

  i_dev->open = usb_controller_open;
  i_dev->close = usb_controller_close;


  controller->pipe = usb_rcvintpipe(dev, ep_irq_in->bEndpointAddress);
  controller->buffer = usb_alloc_coherent(dev, 64, GFP_KERNEL, &controller->input_dma_addr);
  usb_fill_int_urb(controller->my_urb, dev, controller->pipe, controller->buffer, 64, read_callback, controller, ep_irq_in->bEndpointAddress);


  input_set_capability(controller->i_dev, EV_KEY, KEY_SPACE);
  input_set_capability(controller->i_dev, EV_KEY, KEY_B);
  input_set_capability(controller->i_dev, EV_KEY, KEY_X);
  input_set_capability(controller->i_dev, EV_KEY, KEY_Y);
  input_set_capability(controller->i_dev, EV_KEY, KEY_VOLUMEUP);
  input_set_capability(controller->i_dev, EV_KEY, KEY_VOLUMEDOWN);
  input_set_capability(controller->i_dev, EV_KEY, KEY_UP);
  input_set_capability(controller->i_dev, EV_KEY, KEY_DOWN);
  input_set_capability(controller->i_dev, EV_KEY, KEY_LEFT);
  input_set_capability(controller->i_dev, EV_KEY, KEY_RIGHT);

  int err = input_register_device(controller->i_dev);
  if (err) {
    printk(KERN_WARNING "ERROR: Could not input_register_device\n");
    return 1;
  }

  usb_set_intfdata(interface, controller);

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

