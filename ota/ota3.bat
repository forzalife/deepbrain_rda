copy ..\\..\\..\\..\\build\\UNO_81C\\ARM\\main.bin main_ota_1000.bin
copy ..\\..\\..\\..\\build\\UNO_81C\\ARM\\main.bin main_ota_1001.bin
copy ..\\..\\..\\..\\build\\UNO_81C\\ARM\\main.map main_ota.map
python image_pack_firmware.py main_ota_1000.bin 1.0.0.0 0x18006000 0x0
python image_merge.py bootloader.bin 0x1000 main_ota_1000_fwpacked.bin 0x5000 main_merge_ota.bin
python image-pack.py main_ota_1001.bin 1.0.0.1
pause 