# Driver para um controle do Xbox Series S

![ControllerImage](/assets/controller_image.jpg)


## Objetivos
  O trabalho final da matéria de Sistemas Operacionais na UFSC consiste em criar um driver para um dispositivo USB no Linux. Para tal, foi escolhido o controle do Xbox Series S. O objetivodo projeto consiste em entender o protocolo de comunicação USB e utilizar as bibliotecas presentes no Linux para a criação do driver. Basicamente, o fluxo de programa do módulo consiste em:
  * Registrar o dispositivo USB
  * Realizar as comunicações nos endpoints necessários utilizando URB e DMA
  * Processar o input dependendo do dispositivo
  * Informar os inputs para o subsistema de input do Linux


## Funcionamento do driver
  Primeiramente, é preciso encontrar quais são os *endpoints* necessários para a comunicação entre o sistema operacional e o controle. Um *endpoint* é basicamente um buffer de comunicação presente no dispositivo USB que pode enviar dados para o sistema operacional (OUT) ou receber dados dele (IN). Além disso, um endpoint podem ser dos seguintes tipos:
  
  1. *Control*
  2. *Bulk*
  3. *Interrupt*
  4. *Isochronous*

  No caso desse driver, o recebimento de dados acontece via endpoints IN e do tipo *Interrupt*. Para encontrar esse endpoint, pode-se utilizar de softwares como o *usbview*, porém é possível utilizar as próprias funções nativas do Linux para encontrá-los.
  

  Até chegar na parte em que é informado os inputs para o subsistema de input do Linux, várias estrutura de dados devem ser alocadas e inicializadas, as quais acontecem na função de *probe* do código. Posteriormente, as teclas apertadas no controle são mapeadas no sistema operacional da seguinte forma:
  |     Controle         |   Sistema Operacional |
  |    --------          |   -------             |
  |    Botão *A*           |    Espaço             |
  |    Botão *X*           |    *CTRL + C*           |
  |    Botão *B*           |    *CTRL + V*           |
  |    Botão *Y*           |    Apagar             |
  |    Botão *RB*          |    Aumenta Volume     |
  |    Botão *LB*          |    Abaixa Volume      |
  |    Direcional *UP*     |    Seta p/ Cima       |
  |    Direcional *DOWN*   |    Seta p/ Baixo      |
  |    Direcional *LEFT*   |    Seta p/ Esquerda   |
  |    Direcional *RIGHT*  |    Seta p/ Direita    |


## Como utilizar

Primeiramente, é necessário atualizar e instalar os pacotes básicos:
``` 
sudo apt update
sudo apt install build-essential linux-headers-$(uname -r) make git
```

Clone esse repositório:
``` 
git clone https://github.com/lucas-bernardino/driver-controller.git
cd driver-controller
```

Sistemas derivados do Ubuntu/Debian utilizam o *xpad* como driver nativo para a comunicação entre o controle e o sistema operacional. Assim, após conectar o controle na porta USB, é preciso removê-lo para inserir o nosso driver:
```
sudo rmmod xpad
```

Agora, você deve compilar o módulo e inseri-lo no Kernel do Linux:
```
sudo make -C /lib/modules/$(uname -r)/build M=$PWD
sudo insmod modulo.ko
```

Após esses passos, o driver está inserido e funcionando no sistema operacional. Para confirmar, você pode consultar o *dmesg* e verificar as informações correspondentes sobre o idVendor, idProduct e interface do controle:

***controller_driver:*** *O dispositivo idVendor= idProduct= foi connectado ao meu driver, interface=*
```
sudo dmesg | tail
```

