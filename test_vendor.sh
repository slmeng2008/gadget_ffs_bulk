#!/bin/bash

SS_EN=on
USB_FUNCTIONS_DIR=/sys/kernel/config/usb_gadget/rockchip/functions
USB_CONFIGS_DIR=/sys/kernel/config/usb_gadget/rockchip/configs/b.1


pre_run_ss()
{
  if [ $SS_EN = on ];then                                                                                  
    umount /dev/usb-ffs/ss                                                                              
    mkdir -p /dev/usb-ffs/ss -m 0770                                                                       
    mount -o uid=2000,gid=2000 -t functionfs ss /dev/usb-ffs/ss                                         
#    start-stop-daemon --start --quiet --background --exec /oem/aio_simple /dev/usb-ffs/ss/
    /oem/aio_simple /dev/usb-ffs/ss/ &
  fi
}


##main
#init usb config
umount /sys/kernel/config
mkdir /dev/usb-ffs
mount -t configfs none /sys/kernel/config
mkdir -p /sys/kernel/config/usb_gadget/rockchip
mkdir -p /sys/kernel/config/usb_gadget/rockchip/strings/0x409
mkdir -p ${USB_CONFIGS_DIR}/strings/0x409
#echo 0x2207 > /sys/kernel/config/usb_gadget/rockchip/idVendor
echo 0x0483 > /sys/kernel/config/usb_gadget/rockchip/idVendor
echo 0x0310 > /sys/kernel/config/usb_gadget/rockchip/bcdDevice
echo 0x0200 > /sys/kernel/config/usb_gadget/rockchip/bcdUSB
echo "sourcesink" > /sys/kernel/config/usb_gadget/rockchip/strings/0x409/serialnumber
echo "SL" > /sys/kernel/config/usb_gadget/rockchip/strings/0x409/manufacturer
echo "testdevice" > /sys/kernel/config/usb_gadget/rockchip/strings/0x409/product
echo 0x11 > /sys/kernel/config/usb_gadget/rockchip/os_desc/b_vendor_code
echo "MSFT100" > /sys/kernel/config/usb_gadget/rockchip/os_desc/qw_sign
echo "1" > /sys/kernel/config/usb_gadget/rockchip/os_desc/use
echo 500 > /sys/kernel/config/usb_gadget/rockchip/configs/b.1/MaxPower
ln -s /sys/kernel/config/usb_gadget/rockchip/configs/b.1 /sys/kernel/config/usb_gadget/rockchip/os_desc/b.1
#echo 0x0020 > /sys/kernel/config/usb_gadget/rockchip/idProduct
echo 0x0001 > /sys/kernel/config/usb_gadget/rockchip/idProduct


if [ -e ${USB_CONFIGS_DIR}/ffs.ss ]; then
   #for rk1808 kernel 4.4
   rm -f ${USB_CONFIGS_DIR}/ffs.ss
else
   ls ${USB_CONFIGS_DIR} | grep f[0-9] | xargs -I {} rm ${USB_CONFIGS_DIR}/{}
fi

if [ $SS_EN = on ]; then
  mkdir ${USB_FUNCTIONS_DIR}/ffs.ss
  #CONFIG_STR=`cat /sys/kernel/config/usb_gadget/rockchip/configs/b.1/strings/0x409/configuration`
  CONFIG_STR="test"
  STR=${CONFIG_STR}_ss
  echo $STR > ${USB_CONFIGS_DIR}/strings/0x409/configuration
  USB_CNT=`echo $STR | awk -F"_" '{print NF-1}'`
  let USB_CNT=USB_CNT+1
  echo "ss on++++++ ${USB_CNT}"
#  ln -s ${USB_FUNCTIONS_DIR}/ffs.ss ${USB_CONFIGS_DIR}/f${USB_CNT}
  ln -s ${USB_FUNCTIONS_DIR}/ffs.ss ${USB_CONFIGS_DIR}/f1
  pre_run_adb
  sleep 1
fi

UDC=`ls /sys/class/udc/| awk '{print $1}'`
echo $UDC > /sys/kernel/config/usb_gadget/rockchip/UDC

