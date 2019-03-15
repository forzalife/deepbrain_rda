# -*- coding: cp936 -*-
import re
import struct
import time

def get_info(s_src, s_name):
	match_obj = re.search(s_name + "\s+\".+\"", s_src)
	match_obj = re.search("\".+", match_obj.group(0))
	match_obj = re.search("\w+.*\w+", match_obj.group(0))
	
	return match_obj.group()	

if __name__ == "__main__":
	
	#��ͷ�ļ�
	f_head  = open("../deepbrain/apps/userconfig/userconfig.h", "rb")
	str_head = f_head.read()
	
	name    = get_info(str_head, "PROJECT_NAME")
	version = get_info(str_head, "ESP32_FW_VERSION")
	reserve = ""

	print "PROJECT_NAME     : ", name
	print "ESP32_FW_VERSION : ", version
	ss = struct.pack("64s32s160s", name, version, reserve)
	
	combine_name = name[0:17] + version + '.bin'

	#��MVA�ļ�
	f_mva = open("main_ota_1001_fwpacked.bin", "rb") 

	#����combine.MVA�ļ�
	f_combine = open(combine_name, "wb")

	#дͷ
	f_combine.write(ss)

	#дMVA�ļ�
	for line in f_mva:
		f_combine.writelines(line)

	f_combine.flush()
		
	f_head.close
	f_mva.close
	f_combine.close

	print "�ϲ����"	
	
	time.sleep(2)
