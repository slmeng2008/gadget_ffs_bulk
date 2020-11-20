# gadget_ffs_bulk
1. 实现usbdevice与host简单通信
2. 嵌入式设备端，使用linux gadget configfs脚本，configfs+functionfs自定义接口。
3. 提供linuxhost 端测试程序（libusb）
4. 支持windows WCID 免驱动（winusb通用驱动），通过应用访问USB设备（libusb）

> device：
> ./test_vendor.sh

> host:
> ./test
