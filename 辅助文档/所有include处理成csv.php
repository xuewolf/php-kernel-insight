<?php
$c = d();
$c = explode("\n",trim($c));
$file = '';
foreach($c as $v){
	if(strstr($v,"（")){
		$v = trim($v);
		$v = explode("（",$v)[0];
		if(!strstr($v,'php-8.2.5-src')){
			var_dump($v);
		}
		$v = explode("php-8.2.5-src",$v)[2];
		$file = trim($v);
	}
	else{
		$v = trim(explode("/*",trim($v))[0]);
		$v = explode(" ",trim($v));
		$v = trim(array_pop($v),'"');
		echo $file.",".$v."\n";
	}
}
//echo $c;

function d(){
$c = <<<CONTENT
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\bcmath.c （匹配8次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  25: #include "php_ini.h"
	行  26: #include "zend_exceptions.h"
	行  27: #include "bcmath_arginfo.h"
	行  28: #include "ext/standard/info.h"
	行  29: #include "php_bcmath.h"
	行  30: #include "libbcmath/src/bcmath.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\add.c （匹配7次）
	行 32: #include <config.h>
	行 33: #include <stdio.h>
	行 34: #include <stdlib.h>
	行 35: #include <ctype.h>
	行 36: #include <stdarg.h>
	行 37: #include "bcmath.h"
	行 38: #include "private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\bcmath.h （匹配3次）
	行  55: #include "config.h"
	行  58: #include "php.h"
	行  59: #include "../../php_bcmath.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\compare.c （匹配7次）
	行  32: #include <config.h>
	行  33: #include <stdio.h>
	行  34: #include <stdlib.h>
	行  35: #include <ctype.h>
	行  36: #include <stdarg.h>
	行  37: #include "bcmath.h"
	行  38: #include "private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\config.h （匹配6次）
	行  2: #include "../../../../main/config.w32.h"
	行  4: #include <php_config.h>
	行  7: #include "php.h"
	行  8: #include <string.h>
	行  9: #include "zend.h"
	行 10: #include "zend_alloc.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\debug.c （匹配7次）
	行 32: #include <config.h>
	行 33: #include <stdio.h>
	行 34: #include <stdlib.h>
	行 35: #include <ctype.h>
	行 36: #include <stdarg.h>
	行 37: #include "bcmath.h"
	行 38: #include "private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\div.c （匹配7次）
	行  32: #include <config.h>
	行  33: #include <stdio.h>
	行  34: #include <stdlib.h>
	行  35: #include <ctype.h>
	行  36: #include <stdarg.h>
	行  37: #include "bcmath.h"
	行  38: #include "private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\divmod.c （匹配7次）
	行 32: #include <config.h>
	行 33: #include <stdio.h>
	行 34: #include <stdlib.h>
	行 35: #include <ctype.h>
	行 36: #include <stdarg.h>
	行 37: #include "bcmath.h"
	行 38: #include "private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\doaddsub.c （匹配7次）
	行  32: #include <config.h>
	行  33: #include <stdio.h>
	行  34: #include <stdlib.h>
	行  35: #include <ctype.h>
	行  36: #include <stdarg.h>
	行  37: #include "bcmath.h"
	行  38: #include "private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\init.c （匹配7次）
	行  32: #include <config.h>
	行  33: #include <stdio.h>
	行  34: #include <stdlib.h>
	行  35: #include <ctype.h>
	行  36: #include <stdarg.h>
	行  37: #include "bcmath.h"
	行  38: #include "private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\int2num.c （匹配7次）
	行 32: #include <config.h>
	行 33: #include <stdio.h>
	行 34: #include <stdlib.h>
	行 35: #include <ctype.h>
	行 36: #include <stdarg.h>
	行 37: #include "bcmath.h"
	行 38: #include "private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\nearzero.c （匹配7次）
	行 32: #include <config.h>
	行 33: #include <stdio.h>
	行 34: #include <stdlib.h>
	行 35: #include <ctype.h>
	行 36: #include <stdarg.h>
	行 37: #include "bcmath.h"
	行 38: #include "private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\neg.c （匹配7次）
	行 32: #include <config.h>
	行 33: #include <stdio.h>
	行 34: #include <stdlib.h>
	行 35: #include <ctype.h>
	行 36: #include <stdarg.h>
	行 37: #include "bcmath.h"
	行 38: #include "private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\num2long.c （匹配7次）
	行 32: #include <config.h>
	行 33: #include <stdio.h>
	行 34: #include <stdlib.h>
	行 35: #include <ctype.h>
	行 36: #include <stdarg.h>
	行 37: #include "bcmath.h"
	行 38: #include "private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\num2str.c （匹配7次）
	行 32: #include <config.h>
	行 33: #include <stdio.h>
	行 34: #include <stdlib.h>
	行 35: #include <ctype.h>
	行 36: #include <stdarg.h>
	行 37: #include "bcmath.h"
	行 38: #include "private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\output.c （匹配7次）
	行  32: #include <config.h>
	行  33: #include <stdio.h>
	行  34: #include <stdlib.h>
	行  35: #include <ctype.h>
	行  36: #include <stdarg.h>
	行  37: #include "bcmath.h"
	行  38: #include "private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\raise.c （匹配7次）
	行  32: #include <config.h>
	行  33: #include <stdio.h>
	行  34: #include <stdlib.h>
	行  35: #include <ctype.h>
	行  36: #include <stdarg.h>
	行  37: #include "bcmath.h"
	行  38: #include "private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\raisemod.c （匹配8次）
	行  32: #include <config.h>
	行  33: #include <stdio.h>
	行  34: #include <stdlib.h>
	行  35: #include <ctype.h>
	行  36: #include <stdarg.h>
	行  37: #include "bcmath.h"
	行  38: #include "private.h"
	行  39: #include "zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\recmul.c （匹配9次）
	行  32: #include <config.h>
	行  33: #include <stdio.h>
	行  34: #include <assert.h>
	行  35: #include <stdlib.h>
	行  36: #include <ctype.h>
	行  37: #include <stdarg.h>
	行  38: #include "bcmath.h"
	行  39: #include "private.h"
	行  43: #include "muldigits.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\rmzero.c （匹配7次）
	行 32: #include <config.h>
	行 33: #include <stdio.h>
	行 34: #include <stdlib.h>
	行 35: #include <ctype.h>
	行 36: #include <stdarg.h>
	行 37: #include "bcmath.h"
	行 38: #include "private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\sqrt.c （匹配7次）
	行  32: #include <config.h>
	行  33: #include <stdio.h>
	行  34: #include <stdlib.h>
	行  35: #include <ctype.h>
	行  36: #include <stdarg.h>
	行  37: #include "bcmath.h"
	行  38: #include "private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\str2num.c （匹配7次）
	行  32: #include <config.h>
	行  33: #include <stdio.h>
	行  34: #include <stdlib.h>
	行  35: #include <ctype.h>
	行  36: #include <stdarg.h>
	行  37: #include "bcmath.h"
	行  38: #include "private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\sub.c （匹配7次）
	行 32: #include <config.h>
	行 33: #include <stdio.h>
	行 34: #include <stdlib.h>
	行 35: #include <ctype.h>
	行 36: #include <stdarg.h>
	行 37: #include "bcmath.h"
	行 38: #include "private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\libbcmath\\src\\zero.c （匹配7次）
	行 32: #include <config.h>
	行 33: #include <stdio.h>
	行 34: #include <stdlib.h>
	行 35: #include <ctype.h>
	行 36: #include <stdarg.h>
	行 37: #include "bcmath.h"
	行 38: #include "private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bcmath\\php_bcmath.h （匹配2次）
	行 20: #include "libbcmath/src/bcmath.h"
	行 25: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bz2\\bz2.c （匹配8次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  22: #include "php_bz2.h"
	行  23: #include "bz2_arginfo.h"
	行  28: #include "ext/standard/info.h"
	行  29: #include "ext/standard/php_string.h"
	行  30: #include "main/php_network.h"
	行  33: #include <stdio.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bz2\\bz2_filter.c （匹配3次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  22: #include "php_bz2.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\bz2\\php_bz2.h （匹配2次）
	行 26: #include <bzlib.h>
	行 46: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\calendar\\calendar.c （匹配7次）
	行  21: #include "config.h"
	行  24: #include "php.h"
	行  25: #include "ext/standard/info.h"
	行  26: #include "php_calendar.h"
	行  27: #include "sdncal.h"
	行  29: #include <stdio.h>
	行  92: #include "calendar_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\calendar\\cal_unix.c （匹配4次）
	行 19: #include "php.h"
	行 20: #include "php_calendar.h"
	行 21: #include "sdncal.h"
	行 22: #include <time.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\calendar\\dow.c （匹配1次）
	行 31: #include "sdncal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\calendar\\easter.c （匹配4次）
	行  19: #include "php.h"
	行  20: #include "php_calendar.h"
	行  21: #include "sdncal.h"
	行  22: #include <time.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\calendar\\french.c （匹配1次）
	行  87: #include "sdncal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\calendar\\gregor.c （匹配2次）
	行 129: #include "sdncal.h"
	行 130: #include <limits.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\calendar\\jewish.c （匹配1次）
	行 267: #include "sdncal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\calendar\\julian.c （匹配2次）
	行 148: #include "sdncal.h"
	行 149: #include <limits.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\calendar\\php_calendar.h （匹配1次）
	行  7: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\calendar\\sdncal.h （匹配1次）
	行  68: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\com_dotnet\\com_com.c （匹配7次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  22: #include "php_ini.h"
	行  23: #include "ext/standard/info.h"
	行  24: #include "php_com_dotnet.h"
	行  25: #include "php_com_dotnet_internal.h"
	行  26: #include "Zend/zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\com_dotnet\\com_dotnet.c （匹配2次）
	行  18: #include "config.h"
	行  21: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\com_dotnet\\com_extension.c （匹配10次）
	行  18: #include "config.h"
	行  21: #include <intsafe.h>
	行  23: #include "php.h"
	行  24: #include "php_ini.h"
	行  25: #include "ext/standard/info.h"
	行  26: #include "php_com_dotnet.h"
	行  27: #include "php_com_dotnet_internal.h"
	行  28: #include "Zend/zend_exceptions.h"
	行  29: #include "Zend/zend_interfaces.h"
	行  45: #include "com_extension_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\com_dotnet\\com_handlers.c （匹配7次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  22: #include "php_ini.h"
	行  23: #include "ext/standard/info.h"
	行  24: #include "php_com_dotnet.h"
	行  25: #include "php_com_dotnet_internal.h"
	行  26: #include "Zend/zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\com_dotnet\\com_iterator.c （匹配7次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  22: #include "php_ini.h"
	行  23: #include "ext/standard/info.h"
	行  24: #include "php_com_dotnet.h"
	行  25: #include "php_com_dotnet_internal.h"
	行  26: #include "Zend/zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\com_dotnet\\com_misc.c （匹配7次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  22: #include "php_ini.h"
	行  23: #include "ext/standard/info.h"
	行  24: #include "php_com_dotnet.h"
	行  25: #include "php_com_dotnet_internal.h"
	行  26: #include "Zend/zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\com_dotnet\\com_olechar.c （匹配6次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  23: #include "php_ini.h"
	行  24: #include "ext/standard/info.h"
	行  25: #include "php_com_dotnet.h"
	行  26: #include "php_com_dotnet_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\com_dotnet\\com_persist.c （匹配8次）
	行  23: #include "config.h"
	行  26: #include "php.h"
	行  27: #include "php_ini.h"
	行  28: #include "ext/standard/info.h"
	行  29: #include "php_com_dotnet.h"
	行  30: #include "php_com_dotnet_internal.h"
	行  31: #include "Zend/zend_exceptions.h"
	行  32: #include "com_persist_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\com_dotnet\\com_saproxy.c （匹配7次）
	行  25: #include "config.h"
	行  28: #include "php.h"
	行  29: #include "php_ini.h"
	行  30: #include "ext/standard/info.h"
	行  31: #include "php_com_dotnet.h"
	行  32: #include "php_com_dotnet_internal.h"
	行  33: #include "Zend/zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\com_dotnet\\com_typeinfo.c （匹配6次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  23: #include "php_ini.h"
	行  24: #include "ext/standard/info.h"
	行  25: #include "php_com_dotnet.h"
	行  26: #include "php_com_dotnet_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\com_dotnet\\com_variant.c （匹配6次）
	行   18: #include "config.h"
	行   21: #include "php.h"
	行   22: #include "php_ini.h"
	行   23: #include "ext/standard/info.h"
	行   24: #include "php_com_dotnet.h"
	行   25: #include "php_com_dotnet_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\com_dotnet\\com_wrapper.c （匹配6次）
	行  21: #include "config.h"
	行  24: #include "php.h"
	行  25: #include "php_ini.h"
	行  26: #include "ext/standard/info.h"
	行  27: #include "php_com_dotnet.h"
	行  28: #include "php_com_dotnet_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\com_dotnet\\php_com_dotnet.h （匹配2次）
	行 24: #include "TSRM.h"
	行 29: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\com_dotnet\\php_com_dotnet_internal.h （匹配5次）
	行  22: #include <ocidl.h>
	行  23: #include <oleauto.h>
	行  24: #include <unknwn.h>
	行  25: #include <dispex.h>
	行  26: #include "win32/winutil.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\ctype\\ctype.c （匹配8次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  22: #include "php_ini.h"
	行  23: #include "php_ctype.h"
	行  24: #include "ctype_arginfo.h"
	行  25: #include "SAPI.h"
	行  26: #include "ext/standard/info.h"
	行  28: #include <ctype.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\ctype\\php_ctype.h （匹配1次）
	行 20: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\curl\\curl_file.c （匹配4次）
	行  21: #include "php.h"
	行  22: #include "Zend/zend_exceptions.h"
	行  23: #include "curl_private.h"
	行  24: #include "curl_file_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\curl\\curl_private.h （匹配4次）
	行  21: #include "php_curl.h"
	行  25: #include "php_version.h"
	行  28: #include <curl/curl.h>
	行  29: #include <curl/multi.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\curl\\interface.c （匹配15次）
	行   20: #include "config.h"
	行   23: #include "php.h"
	行   24: #include "Zend/zend_exceptions.h"
	行   26: #include <stdio.h>
	行   27: #include <string.h>
	行   30: #include <winsock2.h>
	行   31: #include <sys/types.h>
	行   34: #include <curl/curl.h>
	行   35: #include <curl/easy.h>
	行   58: #include "zend_smart_str.h"
	行   59: #include "ext/standard/info.h"
	行   60: #include "ext/standard/file.h"
	行   61: #include "ext/standard/url.h"
	行   62: #include "curl_private.h"
	行   70: #include "curl_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\curl\\multi.c （匹配10次）
	行  20: #include "config.h"
	行  23: #include "php.h"
	行  24: #include "Zend/zend_smart_str.h"
	行  26: #include "curl_private.h"
	行  28: #include <curl/curl.h>
	行  29: #include <curl/multi.h>
	行  32: #include <sys/select.h>
	行  36: #include <sys/time.h>
	行  40: #include <sys/types.h>
	行  44: #include <unistd.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\curl\\php_curl.h （匹配1次）
	行 21: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\curl\\share.c （匹配4次）
	行  20: #include "config.h"
	行  23: #include "php.h"
	行  25: #include "curl_private.h"
	行  27: #include <curl/curl.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\date\\lib\\astro.c （匹配4次）
	行  29: #include "timelib.h"
	行  30: #include <stdio.h>
	行  31: #include <math.h>
	行  61: #include "astro.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\date\\lib\\dow.c （匹配2次）
	行  25: #include "timelib.h"
	行  26: #include "timelib_private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\date\\lib\\interval.c （匹配3次）
	行  25: #include "timelib.h"
	行  26: #include "timelib_private.h"
	行  27: #include <math.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\date\\lib\\parse_date.c （匹配9次）
	行    28: #include "timelib.h"
	行    29: #include "timelib_private.h"
	行    31: #include <ctype.h>
	行    32: #include <errno.h>
	行    33: #include <math.h>
	行    34: #include <assert.h>
	行    35: #include <limits.h>
	行   160: #include "timezonemap.h"
	行   165: #include "fallbackmap.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\date\\lib\\parse_iso_intervals.c （匹配3次）
	行   27: #include "timelib.h"
	行   28: #include "timelib_private.h"
	行   30: #include <ctype.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\date\\lib\\parse_posix.c （匹配2次）
	行  25: #include "timelib.h"
	行  26: #include "timelib_private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\date\\lib\\parse_tz.c （匹配3次）
	行  26: #include "timelib.h"
	行  27: #include "timelib_private.h"
	行  31: #include "timezonedb.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\date\\lib\\timelib.c （匹配4次）
	行  31: #include "timelib.h"
	行  32: #include "timelib_private.h"
	行  33: #include <ctype.h>
	行  34: #include <math.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\date\\lib\\timelib.h （匹配4次）
	行   37: #include <stdlib.h>
	行   38: #include <stdbool.h>
	行   39: #include <limits.h>
	行   40: #include <inttypes.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\date\\lib\\timelib_private.h （匹配4次）
	行  42: #include <string.h>
	行  45: #include <sys/types.h>
	行  64: #include <stdio.h>
	行  65: #include <limits.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\date\\lib\\tm2unixtime.c （匹配2次）
	行  25: #include "timelib.h"
	行  26: #include "timelib_private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\date\\lib\\unixtime2tm.c （匹配2次）
	行  26: #include "timelib.h"
	行  27: #include "timelib_private.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\date\\php_date.c （匹配17次）
	行   17: #include "php.h"
	行   18: #include "php_streams.h"
	行   19: #include "php_main.h"
	行   20: #include "php_globals.h"
	行   21: #include "php_ini.h"
	行   22: #include "ext/standard/info.h"
	行   23: #include "ext/standard/php_versioning.h"
	行   24: #include "ext/standard/php_math.h"
	行   25: #include "php_date.h"
	行   26: #include "zend_interfaces.h"
	行   27: #include "zend_exceptions.h"
	行   28: #include "lib/timelib.h"
	行   29: #include "lib/timelib_private.h"
	行   31: #include <time.h>
	行   33: #include "win32/time.h"
	行  236: #include "php_date_arginfo.h"
	行  602: #include "zend_smart_str.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\date\\php_date.h （匹配3次）
	行  20: #include "lib/timelib.h"
	行  21: #include "Zend/zend_hash.h"
	行  23: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\dba.c （匹配25次）
	行   19: #include "config.h"
	行   22: #include "php.h"
	行   26: #include "php_ini.h"
	行   27: #include <stdio.h>
	行   28: #include <fcntl.h>
	行   30: #include <sys/file.h>
	行   33: #include "php_dba.h"
	行   34: #include "ext/standard/info.h"
	行   35: #include "ext/standard/php_string.h"
	行   36: #include "ext/standard/flock_compat.h"
	行   38: #include "php_gdbm.h"
	行   39: #include "php_ndbm.h"
	行   40: #include "php_dbm.h"
	行   41: #include "php_cdb.h"
	行   42: #include "php_db1.h"
	行   43: #include "php_db2.h"
	行   44: #include "php_db3.h"
	行   45: #include "php_db4.h"
	行   46: #include "php_flatfile.h"
	行   47: #include "php_inifile.h"
	行   48: #include "php_qdbm.h"
	行   49: #include "php_tcadb.h"
	行   50: #include "php_lmdb.h"
	行   51: #include "dba_arginfo.h"
	行  375: #include "zend_smart_str.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\dba_cdb.c （匹配6次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  25: #include "php_cdb.h"
	行  27: #include <sys/types.h>
	行  29: #include <unistd.h>
	行  31: #include <fcntl.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\dba_db1.c （匹配7次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  24: #include "php_db1.h"
	行  27: #include DB1_INCLUDE_FILE
	行  30: #include <sys/types.h>
	行  31: #include <sys/stat.h>
	行  32: #include <fcntl.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\dba_db2.c （匹配6次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  24: #include "php_db2.h"
	行  25: #include <sys/stat.h>
	行  27: #include <string.h>
	行  29: #include DB2_INCLUDE_FILE
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\dba_db3.c （匹配7次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  24: #include "php_db3.h"
	行  25: #include <sys/stat.h>
	行  27: #include <string.h>
	行  29: #include DB3_INCLUDE_FILE
	行  31: #include <db.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\dba_db4.c （匹配7次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  25: #include "php_db4.h"
	行  26: #include <sys/stat.h>
	行  28: #include <string.h>
	行  30: #include DB4_INCLUDE_FILE
	行  32: #include <db.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\dba_dbm.c （匹配9次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  24: #include "php_dbm.h"
	行  27: #include DBM_INCLUDE_FILE
	行  30: #include "php_gdbm.h"
	行  33: #include <unistd.h>
	行  34: #include <sys/types.h>
	行  35: #include <sys/stat.h>
	行  36: #include <fcntl.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\dba_flatfile.c （匹配8次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  24: #include "php_flatfile.h"
	行  26: #include "libflatfile/flatfile.h"
	行  29: #include <unistd.h>
	行  31: #include <sys/types.h>
	行  32: #include <sys/stat.h>
	行  33: #include <fcntl.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\dba_gdbm.c （匹配4次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  24: #include "php_gdbm.h"
	行  27: #include GDBM_INCLUDE_FILE
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\dba_inifile.c （匹配8次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  24: #include "php_inifile.h"
	行  26: #include "libinifile/inifile.h"
	行  29: #include <unistd.h>
	行  31: #include <sys/types.h>
	行  32: #include <sys/stat.h>
	行  33: #include <fcntl.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\dba_lmdb.c （匹配4次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  24: #include "php_lmdb.h"
	行  27: #include LMDB_INCLUDE_FILE
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\dba_ndbm.c （匹配5次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  24: #include "php_ndbm.h"
	行  26: #include <fcntl.h>
	行  28: #include NDBM_INCLUDE_FILE
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\dba_qdbm.c （匹配4次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  24: #include "php_qdbm.h"
	行  27: #include QDBM_INCLUDE_FILE
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\dba_tcadb.c （匹配4次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  24: #include "php_tcadb.h"
	行  27: #include TCADB_INCLUDE_FILE
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\libcdb\\cdb.c （匹配9次）
	行  22: #include "config.h"
	行  25: #include "php.h"
	行  27: #include <sys/types.h>
	行  28: #include <sys/stat.h>
	行  30: #include <sys/mman.h>
	行  33: #include <unistd.h>
	行  35: #include <string.h>
	行  36: #include <errno.h>
	行  37: #include "cdb.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\libcdb\\cdb.h （匹配1次）
	行 22: #include "uint32.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\libcdb\\cdb_make.c （匹配10次）
	行  22: #include "config.h"
	行  25: #include "php.h"
	行  27: #include <sys/types.h>
	行  29: #include <unistd.h>
	行  31: #include <stdlib.h>
	行  32: #include <stdio.h>
	行  33: #include <errno.h>
	行  34: #include "cdb.h"
	行  35: #include "cdb_make.h"
	行  36: #include "uint32.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\libcdb\\cdb_make.h （匹配2次）
	行 22: #include <stdio.h>
	行 23: #include "uint32.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\libcdb\\uint32.c （匹配3次）
	行 20: #include "config.h"
	行 23: #include "php.h"
	行 25: #include "uint32.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\libflatfile\\flatfile.c （匹配8次）
	行  23: #include "config.h"
	行  26: #include "php.h"
	行  27: #include "php_globals.h"
	行  29: #include <stdlib.h>
	行  30: #include <string.h>
	行  31: #include <errno.h>
	行  33: #include <unistd.h>
	行  36: #include "flatfile.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\libinifile\\inifile.c （匹配8次）
	行  20: #include "config.h"
	行  23: #include "php.h"
	行  24: #include "php_globals.h"
	行  26: #include <stdlib.h>
	行  27: #include <string.h>
	行  28: #include <errno.h>
	行  30: #include <unistd.h>
	行  33: #include "inifile.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\php_cdb.h （匹配1次）
	行  6: #include "php_dba.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\php_db1.h （匹配1次）
	行  6: #include "php_dba.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\php_db2.h （匹配1次）
	行  6: #include "php_dba.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\php_db3.h （匹配1次）
	行  6: #include "php_dba.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\php_db4.h （匹配1次）
	行  6: #include "php_dba.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\php_dba.h （匹配1次）
	行  20: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\php_dbm.h （匹配1次）
	行  6: #include "php_dba.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\php_flatfile.h （匹配1次）
	行  6: #include "php_dba.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\php_gdbm.h （匹配1次）
	行  6: #include "php_dba.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\php_inifile.h （匹配1次）
	行  6: #include "php_dba.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\php_lmdb.h （匹配2次）
	行  6: #include "php_dba.h"
	行  7: #include <lmdb.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\php_ndbm.h （匹配1次）
	行  6: #include "php_dba.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\php_qdbm.h （匹配1次）
	行  6: #include "php_dba.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dba\\php_tcadb.h （匹配1次）
	行 22: #include "php_dba.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dl_test\\dl_test.c （匹配4次）
	行  21: #include "php.h"
	行  22: #include "ext/standard/info.h"
	行  23: #include "php_dl_test.h"
	行  24: #include "dl_test_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\attr.c （匹配3次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  26: #include "php_dom.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\cdatasection.c （匹配3次）
	行 19: #include "config.h"
	行 22: #include "php.h"
	行 24: #include "php_dom.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\characterdata.c （匹配3次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  24: #include "php_dom.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\comment.c （匹配3次）
	行 19: #include "config.h"
	行 22: #include "php.h"
	行 24: #include "php_dom.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\document.c （匹配6次）
	行   19: #include "config.h"
	行   22: #include "php.h"
	行   24: #include "php_dom.h"
	行   25: #include <libxml/SAX.h>
	行   27: #include <libxml/relaxng.h>
	行   28: #include <libxml/xmlschemas.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\documentfragment.c （匹配3次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  24: #include "php_dom.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\documenttype.c （匹配3次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  24: #include "php_dom.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\domexception.c （匹配3次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  24: #include "php_dom.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\domimplementation.c （匹配3次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  24: #include "php_dom.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\dom_iterators.c （匹配4次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  24: #include "php_dom.h"
	行  25: #include "dom_ce.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\element.c （匹配3次）
	行   19: #include "config.h"
	行   22: #include "php.h"
	行   24: #include "php_dom.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\entity.c （匹配3次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  24: #include "php_dom.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\entityreference.c （匹配3次）
	行 19: #include "config.h"
	行 22: #include "php.h"
	行 24: #include "php_dom.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\namednodemap.c （匹配4次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  24: #include "php_dom.h"
	行  25: #include "zend_interfaces.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\node.c （匹配3次）
	行   19: #include "config.h"
	行   22: #include "php.h"
	行   24: #include "php_dom.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\nodelist.c （匹配4次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  24: #include "php_dom.h"
	行  25: #include "zend_interfaces.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\notation.c （匹配3次）
	行 19: #include "config.h"
	行 22: #include "php.h"
	行 24: #include "php_dom.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\parentnode.c （匹配3次）
	行  20: #include "config.h"
	行  23: #include "php.h"
	行  25: #include "php_dom.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\php_dom.c （匹配8次）
	行   20: #include "config.h"
	行   23: #include "php.h"
	行   25: #include "ext/random/php_random.h"
	行   26: #include "php_dom.h"
	行   27: #include "php_dom_arginfo.h"
	行   28: #include "dom_properties.h"
	行   29: #include "zend_interfaces.h"
	行   31: #include "ext/standard/info.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\php_dom.h （匹配19次）
	行  26: #include "TSRM.h"
	行  29: #include <libxml/parser.h>
	行  30: #include <libxml/parserInternals.h>
	行  31: #include <libxml/tree.h>
	行  32: #include <libxml/uri.h>
	行  33: #include <libxml/xmlerror.h>
	行  34: #include <libxml/xinclude.h>
	行  35: #include <libxml/hash.h>
	行  36: #include <libxml/c14n.h>
	行  38: #include <libxml/HTMLparser.h>
	行  39: #include <libxml/HTMLtree.h>
	行  42: #include <libxml/xpath.h>
	行  43: #include <libxml/xpathInternals.h>
	行  46: #include <libxml/xpointer.h>
	行  54: #include "xml_common.h"
	行  55: #include "ext/libxml/php_libxml.h"
	行  56: #include "zend_exceptions.h"
	行  57: #include "dom_ce.h"
	行  96: #include "domexception.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\processinginstruction.c （匹配3次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  24: #include "php_dom.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\text.c （匹配4次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  24: #include "php_dom.h"
	行  25: #include "dom_ce.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\xml_common.h （匹配1次）
	行 21: #include "ext/libxml/php_libxml.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\dom\\xpath.c （匹配3次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  24: #include "php_dom.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\enchant\\enchant.c （匹配9次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  23: #include "php_ini.h"
	行  24: #include "ext/standard/info.h"
	行  25: #include "Zend/zend_exceptions.h"
	行  26: #include "../spl/spl_exceptions.h"
	行  27: #include <enchant.h>
	行  28: #include "php_enchant.h"
	行  36: #include "enchant_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\enchant\\php_enchant.h （匹配1次）
	行 32: #include "TSRM.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\exif\\exif.c （匹配11次）
	行   19: #include "config.h"
	行   22: #include "php.h"
	行   23: #include "ext/standard/file.h"
	行   42: #include "php_exif.h"
	行   43: #include "exif_arginfo.h"
	行   44: #include <math.h>
	行   45: #include "php_ini.h"
	行   46: #include "ext/standard/php_string.h"
	行   47: #include "ext/standard/php_image.h"
	行   48: #include "ext/standard/info.h"
	行   51: #include <sys/types.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\exif\\php_exif.h （匹配1次）
	行 18: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\ffi\\ffi.c （匹配14次）
	行   21: #include "php.h"
	行   22: #include "php_ffi.h"
	行   23: #include "ext/standard/info.h"
	行   24: #include "php_scandir.h"
	行   25: #include "zend_exceptions.h"
	行   26: #include "zend_closures.h"
	行   27: #include "main/SAPI.h"
	行   29: #include <ffi.h>
	行   31: #include <sys/types.h>
	行   32: #include <sys/stat.h>
	行   33: #include <fcntl.h>
	行   37: #include "win32/glob.h"
	行   39: #include <glob.h>
	行   88: #include "ffi_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\ffi\\ffi_parser.c （匹配5次）
	行   21: #include "php.h"
	行   22: #include "php_ffi.h"
	行   24: #include <stdio.h>
	行   25: #include <stdlib.h>
	行   26: #include <string.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\fileinfo.c （匹配10次）
	行  18: #include "config.h"
	行  20: #include "php.h"
	行  22: #include "libmagic/magic.h"
	行  31: #include "php_ini.h"
	行  32: #include "ext/standard/info.h"
	行  33: #include "ext/standard/file.h" /* needed for context stuff */
	行  34: #include "php_fileinfo.h"
	行  35: #include "fileinfo_arginfo.h"
	行  36: #include "fopen_wrappers.h" /* needed for is_url */
	行  37: #include "Zend/zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\apprentice.c （匹配11次）
	行   32: #include "php.h"
	行   34: #include "file.h"
	行   40: #include "magic.h"
	行   41: #include <stdlib.h>
	行   52: #include "win32/unistd.h"
	行   56: #include <unistd.h>
	行   59: #include <string.h>
	行   60: #include <assert.h>
	行   61: #include <ctype.h>
	行   62: #include <fcntl.h>
	行  187: #include "../data_file.c"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\apptype.c （匹配5次）
	行  27: #include "file.h"
	行  33: #include <stdlib.h>
	行  34: #include <string.h>
	行  37: #include <io.h>
	行  41: #include <os2.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\ascmagic.c （匹配6次）
	行  35: #include "file.h"
	行  41: #include "magic.h"
	行  42: #include <string.h>
	行  43: #include <ctype.h>
	行  44: #include <stdlib.h>
	行  46: #include <unistd.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\buffer.c （匹配7次）
	行 27: #include "file.h"
	行 33: #include "magic.h"
	行 35: #include "win32/unistd.h"
	行 37: #include <unistd.h>
	行 39: #include <string.h>
	行 40: #include <stdlib.h>
	行 41: #include <sys/stat.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\cdf.c （匹配11次）
	行   35: #include "file.h"
	行   41: #include <assert.h>
	行   43: #include <err.h>
	行   45: #include <stdlib.h>
	行   48: #include "win32/unistd.h"
	行   50: #include <unistd.h>
	行   57: #include <string.h>
	行   58: #include <time.h>
	行   59: #include <ctype.h>
	行   60: #include <limits.h>
	行   70: #include "cdf.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\cdf.h （匹配1次）
	行  39: #include <winsock2.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\cdf_time.c （匹配6次）
	行  26: #include "php.h"
	行  28: #include "file.h"
	行  34: #include <time.h>
	行  36: #include <err.h>
	行  38: #include <string.h>
	行  40: #include "cdf.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\compress.c （匹配15次）
	行  35: #include "file.h"
	行  41: #include "magic.h"
	行  42: #include <stdlib.h>
	行  44: #include <unistd.h>
	行  46: #include <string.h>
	行  47: #include <errno.h>
	行  48: #include <ctype.h>
	行  49: #include <stdarg.h>
	行  50: #include <signal.h>
	行  55: #include <sys/ioctl.h>
	行  58: #include <sys/wait.h>
	行  61: #include <sys/time.h>
	行  65: #include <zlib.h>
	行  72: #include <bzlib.h>
	行  77: #include <lzma.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\config.h （匹配1次）
	行 1: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\der.c （匹配12次）
	行  35: #include "file.h"
	行  45: #include <sys/types.h>
	行  47: #include <stdio.h>
	行  48: #include <fcntl.h>
	行  49: #include <stdlib.h>
	行  50: #include <string.h>
	行  51: #include <ctype.h>
	行  54: #include "magic.h"
	行  55: #include "der.h"
	行  58: #include <sys/mman.h>
	行  60: #include <sys/stat.h>
	行  61: #include <err.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\encoding.c （匹配4次）
	行  35: #include "file.h"
	行  41: #include "magic.h"
	行  42: #include <string.h>
	行  43: #include <stdlib.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\file.h （匹配14次）
	行  36: #include "config.h"
	行  38: #include "php.h"
	行  39: #include "ext/standard/php_string.h"
	行  40: #include "ext/pcre/php_pcre.h"
	行  42: #include <stdint.h>
	行  43: #include <inttypes.h>
	行  76: #include <stdio.h>	/* Include that here, to make sure __P gets defined */
	行  77: #include <errno.h>
	行  78: #include <fcntl.h>	/* For open and flags */
	行  80: #include <sys/types.h>
	行  82: #include "win32/param.h"
	行  84: #include <sys/param.h>
	行  87: #include <sys/stat.h>
	行  88: #include <stdarg.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\fsmagic.c （匹配5次）
	行  32: #include "file.h"
	行  38: #include "magic.h"
	行  39: #include <string.h>
	行  41: #include <unistd.h>
	行  43: #include <stdlib.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\funcs.c （匹配13次）
	行  27: #include "file.h"
	行  33: #include "magic.h"
	行  34: #include <assert.h>
	行  35: #include <stdarg.h>
	行  36: #include <stdlib.h>
	行  37: #include <string.h>
	行  38: #include <ctype.h>
	行  40: #include <unistd.h>	/* for pipe2() */
	行  43: #include <wchar.h>
	行  46: #include <wctype.h>
	行  48: #include <limits.h>
	行  54: #include "php.h"
	行  55: #include "main/php_network.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\is_csv.c （匹配13次）
	行  32: #include "file.h"
	行  38: #include <string.h>
	行  39: #include "magic.h"
	行  41: #include <sys/types.h>
	行  46: #include <stdio.h>
	行 165: #include <sys/types.h>
	行 166: #include <sys/stat.h>
	行 167: #include <stdio.h>
	行 168: #include <fcntl.h>
	行 169: #include <unistd.h>
	行 170: #include <stdlib.h>
	行 171: #include <stdint.h>
	行 172: #include <err.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\is_json.c （匹配12次）
	行  32: #include "file.h"
	行  38: #include <string.h>
	行  39: #include "magic.h"
	行  43: #include <stdio.h>
	行 435: #include <sys/types.h>
	行 436: #include <sys/stat.h>
	行 437: #include <stdio.h>
	行 438: #include <fcntl.h>
	行 439: #include <unistd.h>
	行 440: #include <stdlib.h>
	行 441: #include <stdint.h>
	行 442: #include <err.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\is_tar.c （匹配5次）
	行  40: #include "file.h"
	行  46: #include "magic.h"
	行  47: #include <string.h>
	行  48: #include <ctype.h>
	行  49: #include "tar.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\magic.c （匹配10次）
	行  28: #include "file.h"
	行  34: #include "magic.h"
	行  36: #include <stdlib.h>
	行  38: #include "win32/unistd.h"
	行  40: #include <unistd.h>
	行  42: #include <string.h>
	行  43: #include "config.h"
	行  46: #include <shlwapi.h>
	行  48: #include <limits.h>	/* for PIPE_BUF */
	行  61: #include <unistd.h>	/* for read() */
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\magic.h （匹配1次）
	行  30: #include <sys/types.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\print.c （匹配8次）
	行  31: #include "php.h"
	行  33: #include "file.h"
	行  39: #include <string.h>
	行  40: #include <stdarg.h>
	行  41: #include <stdlib.h>
	行  43: #include <unistd.h>
	行  45: #include <time.h>
	行  47: #include "cdf.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\readcdf.c （匹配10次）
	行  26: #include "file.h"
	行  32: #include <assert.h>
	行  33: #include <stdlib.h>
	行  35: #include "win32/unistd.h"
	行  37: #include <unistd.h>
	行  39: #include <string.h>
	行  40: #include <time.h>
	行  41: #include <ctype.h>
	行  43: #include "cdf.h"
	行  44: #include "magic.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\softmagic.c （匹配8次）
	行   32: #include "file.h"
	行   38: #include "magic.h"
	行   39: #include <assert.h>
	行   40: #include <string.h>
	行   41: #include <ctype.h>
	行   42: #include <stdlib.h>
	行   43: #include <time.h>
	行   44: #include "der.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\libmagic\\strcasestr.c （匹配6次）
	行 40: #include "file.h"
	行 42: #include <inttypes.h>
	行 43: #include <stdint.h>
	行 44: #include <assert.h>
	行 45: #include <ctype.h>
	行 46: #include <string.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\fileinfo\\php_fileinfo.h （匹配1次）
	行 32: #include "TSRM.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\filter\\callback_filter.c （匹配1次）
	行 17: #include "php_filter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\filter\\filter.c （匹配4次）
	行  21: #include "config.h"
	行  24: #include "php_filter.h"
	行  28: #include "filter_private.h"
	行  29: #include "filter_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\filter\\logical_filters.c （匹配5次）
	行   19: #include "php_filter.h"
	行   20: #include "filter_private.h"
	行   21: #include "ext/standard/url.h"
	行   22: #include "ext/pcre/php_pcre.h"
	行   24: #include "zend_multiply.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\filter\\php_filter.h （匹配9次）
	行 21: #include "SAPI.h"
	行 22: #include "zend_API.h"
	行 23: #include "php.h"
	行 24: #include "php_ini.h"
	行 25: #include "ext/standard/info.h"
	行 26: #include "ext/standard/php_string.h"
	行 27: #include "ext/standard/html.h"
	行 28: #include "php_variables.h"
	行 34: #include "TSRM.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\filter\\sanitizing_filters.c （匹配3次）
	行  17: #include "php_filter.h"
	行  18: #include "filter_private.h"
	行  19: #include "zend_smart_str.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\ftp\\ftp.c （匹配22次）
	行   19: #include "config.h"
	行   22: #include "php.h"
	行   24: #include <stdio.h>
	行   25: #include <ctype.h>
	行   26: #include <stdlib.h>
	行   28: #include <unistd.h>
	行   30: #include <fcntl.h>
	行   31: #include <string.h>
	行   32: #include <time.h>
	行   34: #include <winsock2.h>
	行   37: #include <sys/types.h>
	行   39: #include <sys/socket.h>
	行   40: #include <netinet/in.h>
	行   41: #include <arpa/inet.h>
	行   42: #include <netdb.h>
	行   44: #include <errno.h>
	行   47: #include <sys/time.h>
	行   51: #include <sys/select.h>
	行   55: #include <openssl/ssl.h>
	行   56: #include <openssl/err.h>
	行   59: #include "ftp.h"
	行   60: #include "ext/standard/fsock.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\ftp\\ftp.h （匹配3次）
	行  21: #include "php_network.h"
	行  23: #include <stdio.h>
	行  25: #include <netinet/in.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\ftp\\php_ftp.c （匹配9次）
	行   19: #include "config.h"
	行   22: #include "php.h"
	行   30: #include "ext/standard/info.h"
	行   31: #include "ext/standard/file.h"
	行   32: #include "Zend/zend_attributes.h"
	行   33: #include "Zend/zend_exceptions.h"
	行   35: #include "php_ftp.h"
	行   36: #include "ftp.h"
	行   37: #include "ftp_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\ftp\\php_ftp.h （匹配1次）
	行 24: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\gd.c （匹配14次）
	行   25: #include "config.h"
	行   28: #include "php.h"
	行   29: #include "php_ini.h"
	行   30: #include "ext/standard/head.h"
	行   31: #include <math.h>
	行   32: #include "SAPI.h"
	行   33: #include "php_gd.h"
	行   34: #include "ext/standard/php_image.h"
	行   35: #include "ext/standard/info.h"
	行   36: #include "php_open_temporary_file.h"
	行   37: #include "php_memory_streams.h"
	行   38: #include "zend_object_handlers.h"
	行   58: #include "gd_compat.h"
	行  100: #include "gd_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\gd_compat.c （匹配4次）
	行  2: #include "config.h"
	行  4: #include "php_config.h"
	行  7: #include "gd_compat.h"
	行  8: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd.c （匹配7次）
	行    2: #include <math.h>
	行    3: #include <string.h>
	行    4: #include <stdlib.h>
	行    5: #include "gd.h"
	行    6: #include "gdhelpers.h"
	行    7: #include "gd_errors.h"
	行    9: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd.h （匹配5次）
	行   9: #include "config.h"
	行  12: #include "php_compat.h"
	行  46: #include <stdio.h>
	行  47: #include "gd_io.h"
	行  50: #include <stdarg.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd2copypal.c （匹配3次）
	行  2: #include <stdio.h>
	行  3: #include "gd.h"
	行  4: #include <stdlib.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd2time.c （匹配4次）
	行  2: #include <stdio.h>
	行  3: #include <stdlib.h>		/* for atoi */
	行  4: #include <time.h>		/* For time */
	行  5: #include "gd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd2topng.c （匹配2次）
	行  2: #include <stdio.h>
	行  3: #include "gd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gdcache.c （匹配4次）
	行   1: #include "gd.h"
	行   2: #include "gdhelpers.h"
	行  48: #include "gdcache.h"
	行 149: #include <stdio.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gdcache.h （匹配2次）
	行 43: #include <stdlib.h>
	行 45:  #include <malloc.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gddemo.c （匹配4次）
	行   1: #include <stdio.h>
	行   2: #include "gd.h"
	行   3: #include "gdfontg.h"
	行   4: #include "gdfonts.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gdfontg.c （匹配1次）
	行   14: #include "gdfontg.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gdfontg.h （匹配1次）
	行 20: #include "gd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gdfontl.c （匹配1次）
	行   15: #include "gdfontl.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gdfontl.h （匹配1次）
	行 21: #include "gd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gdfontmb.c （匹配1次）
	行   13: #include "gdfontmb.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gdfontmb.h （匹配1次）
	行 19: #include "gd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gdfonts.c （匹配1次）
	行   13: #include "gdfonts.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gdfonts.h （匹配1次）
	行 19: #include "gd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gdfontt.c （匹配1次）
	行   14: #include "gdfontt.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gdfontt.h （匹配1次）
	行 20: #include "gd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gdft.c （匹配13次）
	行    8: #include <stdio.h>
	行    9: #include <stdlib.h>
	行   10: #include <string.h>
	行   11: #include <math.h>
	行   12: #include "gd.h"
	行   13: #include "gdhelpers.h"
	行   16: #include <unistd.h>
	行   18: #include <io.h>
	行   63: #include "gdcache.h"
	行   64: #include <ft2build.h>
	行   65: #include FT_FREETYPE_H
	行   66: #include FT_GLYPH_H
	行  193: #include "jisx0208.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gdhelpers.c （匹配5次）
	行  2: #include "config.h"
	行  5: #include "gd.h"
	行  6: #include "gdhelpers.h"
	行  7: #include <stdlib.h>
	行  8: #include <string.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gdhelpers.h （匹配2次）
	行  4: #include <sys/types.h>
	行  5: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gdkanji.c （匹配8次）
	行   5: #include <stdio.h>
	行   6: #include <stdlib.h>
	行   7: #include <string.h>
	行   8: #include "gd.h"
	行   9: #include "gdhelpers.h"
	行  11: #include <stdarg.h>
	行  13: #include <iconv.h>
	行  14: #include <errno.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gdparttopng.c （匹配3次）
	行  1: #include <stdio.h>
	行  2: #include <stdlib.h>		/* For atoi */
	行  3: #include "gd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gdtables.c （匹配2次）
	行   3: #include "config.h"
	行   6: #include "php_compat.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gdtest.c （匹配4次）
	行   1: #include <stdio.h>
	行   3: #include <process.h>
	行  10: #include <unistd.h>		/* for getpid(), unlink() */
	行  12: #include "gd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gdtestft.c （匹配2次）
	行   2: #include "gd.h"
	行   3: #include <string.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gdtopng.c （匹配2次）
	行  1: #include <stdio.h>
	行  2: #include "gd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gdxpm.c （匹配6次）
	行   8: #include <stdio.h>
	行   9: #include <stdlib.h>
	行  10: #include <string.h>
	行  11: #include "gd.h"
	行  12: #include "gdhelpers.h"
	行  16: #include <X11/xpm.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_avif.c （匹配11次）
	行   2: #include "config.h"
	行   5: #include <stdio.h>
	行   6: #include <stdlib.h>
	行   7: #include <string.h>
	行   8: #include <limits.h>
	行   9: #include <math.h>
	行  11: #include "gd.h"
	行  12: #include "gd_errors.h"
	行  13: #include "gdhelpers.h"
	行  14: #include "gd_intern.h"
	行  17: #include <avif/avif.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_bmp.c （匹配8次）
	行   17: #include "config.h"
	行   20: #include <stdio.h>
	行   21: #include <math.h>
	行   22: #include <string.h>
	行   23: #include <stdlib.h>
	行   24: #include "gd.h"
	行   25: #include "gdhelpers.h"
	行   26: #include "bmp.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_color_match.c （匹配4次）
	行  1: #include "gd.h"
	行  2: #include "gdhelpers.h"
	行  4: #include "gd_intern.h"
	行  5: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_crop.c （匹配4次）
	行  22: #include <stdlib.h>
	行  23: #include <string.h>
	行  24: #include <math.h>
	行  26: #include "gd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_filter.c （匹配4次）
	行   1: #include "gd.h"
	行   3: #include "gd_intern.h"
	行  10: #include <stdlib.h>
	行  11: #include <time.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_gd.c （匹配5次）
	行   1: #include <stdio.h>
	行   2: #include <math.h>
	行   3: #include <string.h>
	行   4: #include <stdlib.h>
	行   5: #include "gd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_gd2.c （匹配9次）
	行  13: #include <stdio.h>
	行  14: #include <errno.h>
	行  15: #include <math.h>
	行  16: #include <string.h>
	行  17: #include <stdlib.h>
	行  18: #include "gd.h"
	行  19: #include "gd_errors.h"
	行  20: #include "gdhelpers.h"
	行  22: #include <zlib.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_gif_in.c （匹配7次）
	行   1: #include <stdio.h>
	行   2: #include <math.h>
	行   3: #include <string.h>
	行   4: #include <stdlib.h>
	行   5: #include "gd.h"
	行   6: #include "gd_errors.h"
	行   8: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_gif_out.c （匹配6次）
	行   1: #include <stdio.h>
	行   2: #include <math.h>
	行   3: #include <string.h>
	行   4: #include <stdlib.h>
	行   5: #include "gd.h"
	行 459: #include <ctype.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_interpolation.c （匹配7次）
	行   56: #include <stdio.h>
	行   57: #include <stdlib.h>
	行   58: #include <string.h>
	行   59: #include <math.h>
	行   61: #include "gd.h"
	行   62: #include "gdhelpers.h"
	行   63: #include "gd_intern.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_io.c （匹配4次）
	行  15: #include <math.h>
	行  16: #include <string.h>
	行  17: #include <stdlib.h>
	行  18: #include "gd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_io.h （匹配1次）
	行  4: #include <stdio.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_io_dp.c （匹配5次）
	行  19: #include <math.h>
	行  20: #include <string.h>
	行  21: #include <stdlib.h>
	行  22: #include "gd.h"
	行  23: #include "gdhelpers.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_io_file.c （匹配5次）
	行  23: #include <math.h>
	行  24: #include <string.h>
	行  25: #include <stdlib.h>
	行  26: #include "gd.h"
	行  27: #include "gdhelpers.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_io_ss.c （匹配5次）
	行  24: #include <math.h>
	行  25: #include <string.h>
	行  26: #include <stdlib.h>
	行  27: #include "gd.h"
	行  28: #include "gdhelpers.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_jpeg.c （匹配10次）
	行  24: #include <stdio.h>
	行  25: #include <stdlib.h>
	行  26: #include <setjmp.h>
	行  27: #include <limits.h>
	行  28: #include <string.h>
	行  30: #include "gd.h"
	行  31: #include "gd_errors.h"
	行  35: #include "gdhelpers.h"
	行  39: #include "jpeglib.h"
	行  40: #include "jerror.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_matrix.c （匹配2次）
	行   1: #include "gd.h"
	行   2: #include <math.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_pixelate.c （匹配1次）
	行  1: #include "gd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_png.c （匹配8次）
	行   1: #include <stdio.h>
	行   2: #include <math.h>
	行   3: #include <string.h>
	行   4: #include <stdlib.h>
	行   5: #include "gd.h"
	行   6: #include "gd_errors.h"
	行  11: #include "png.h"		/* includes zlib.h and setjmp.h */
	行  12: #include "gdhelpers.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_rotate.c （匹配3次）
	行   1: #include "gd.h"
	行   3: #include "gd_intern.h"
	行   4: #include <math.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_security.c （匹配6次）
	行 12: #include "config.h"
	行 15: #include <stdio.h>
	行 16: #include <stdlib.h>
	行 17: #include <limits.h>
	行 18: #include "gd.h"
	行 19: #include "gd_errors.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_ss.c （匹配5次）
	行  1: #include <stdio.h>
	行  2: #include <math.h>
	行  3: #include <string.h>
	行  4: #include <stdlib.h>
	行  5: #include "gd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_tga.c （匹配9次）
	行   8: #include "config.h"
	行  11: #include <stdio.h>
	行  12: #include <stddef.h>
	行  13: #include <stdlib.h>
	行  14: #include <string.h>
	行  16: #include "gd_tga.h"
	行  17: #include "gd.h"
	行  18: #include "gd_errors.h"
	行  19: #include "gdhelpers.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_tga.h （匹配3次）
	行  4: #include "gd.h"
	行  5: #include "gdhelpers.h"
	行  7: #include "gd_intern.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_topal.c （匹配3次）
	行   38: #include <string.h>
	行   39: #include "gd.h"
	行   40: #include "gdhelpers.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_transform.c （匹配1次）
	行  1: #include "gd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_wbmp.c （匹配7次）
	行  54: #include <stdio.h>
	行  55: #include <stdlib.h>
	行  56: #include <limits.h>
	行  58: #include "gd.h"
	行  59: #include "gdfonts.h"
	行  60: #include "gd_errors.h"
	行  61: #include "wbmp.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_webp.c （匹配8次）
	行   1: #include <stdio.h>
	行   2: #include <math.h>
	行   3: #include <string.h>
	行   4: #include <stdlib.h>
	行   5: #include "gd.h"
	行   6: #include "gdhelpers.h"
	行   9: #include "webp/decode.h"
	行  10: #include "webp/encode.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\gd_xbm.c （匹配8次）
	行  19: #include <stdio.h>
	行  20: #include <math.h>
	行  21: #include <string.h>
	行  22: #include <stdlib.h>
	行  23: #include "gd.h"
	行  24: #include "gdhelpers.h"
	行  25: #include "gd_errors.h"
	行  27: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\pngtogd.c （匹配2次）
	行  2: #include <stdio.h>
	行  3: #include "gd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\pngtogd2.c （匹配3次）
	行  2: #include <stdio.h>
	行  3: #include <stdlib.h>
	行  4: #include "gd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\testac.c （匹配2次）
	行   2: #include <stdio.h>
	行   3: #include "gd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\wbmp.c （匹配7次）
	行  13: #include <stdio.h>
	行  14: #include <stddef.h>
	行  15: #include <stdlib.h>
	行  16: #include <string.h>
	行  18: #include "wbmp.h"
	行  19: #include "gd.h"
	行  20: #include "gdhelpers.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\wbmp.h （匹配2次）
	行 16: #include "config.h"
	行 19: #include "php_compat.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\libgd\\webpng.c （匹配6次）
	行   2: #include "gd.h"
	行   5: #include <stdio.h>
	行   6: #include <stdlib.h>		/* for atoi() */
	行   7: #include <string.h>
	行  10: #include <process.h>
	行  17: #include <unistd.h>		/* for getpid(), unlink() */
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gd\\php_gd.h （匹配3次）
	行  21: #include "zend_string.h"
	行  22: #include "php_streams.h"
	行 110: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gettext\\gettext.c （匹配7次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  25: #include <stdio.h>
	行  26: #include "ext/standard/info.h"
	行  27: #include "php_gettext.h"
	行  28: #include "gettext_arginfo.h"
	行  30: #include <libintl.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gettext\\php_gettext.h （匹配1次）
	行 25: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gmp\\gmp.c （匹配12次）
	行   18: #include "config.h"
	行   21: #include "php.h"
	行   22: #include "php_ini.h"
	行   23: #include "php_gmp.h"
	行   24: #include "php_gmp_int.h"
	行   25: #include "ext/standard/info.h"
	行   26: #include "ext/standard/php_var.h"
	行   27: #include "zend_smart_str_public.h"
	行   28: #include "zend_exceptions.h"
	行   30: #include <gmp.h>
	行   33: #include "ext/random/php_random.h"
	行   50: #include "gmp_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gmp\\php_gmp.h （匹配2次）
	行 20: #include <gmp.h>
	行 25: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\gmp\\php_gmp_int.h （匹配3次）
	行  5: #include "config.h"
	行  8: #include "php.h"
	行  9: #include <gmp.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\hash.c （匹配12次）
	行   19: #include "config.h"
	行   22: #include <math.h>
	行   23: #include "php_hash.h"
	行   24: #include "ext/standard/info.h"
	行   25: #include "ext/standard/file.h"
	行   26: #include "ext/standard/php_var.h"
	行   27: #include "ext/spl/spl_exceptions.h"
	行   29: #include "zend_attributes.h"
	行   30: #include "zend_exceptions.h"
	行   31: #include "zend_interfaces.h"
	行   32: #include "zend_smart_str.h"
	行   34: #include "hash_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\hash_adler32.c （匹配2次）
	行 18: #include "php_hash.h"
	行 19: #include "php_hash_adler32.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\hash_crc32.c （匹配4次）
	行  18: #include "php_hash.h"
	行  19: #include "php_hash_crc32.h"
	行  20: #include "php_hash_crc32_tables.h"
	行  21: #include "ext/standard/crc32_x86.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\hash_fnv.c （匹配2次）
	行  20: #include "php_hash.h"
	行  21: #include "php_hash_fnv.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\hash_gost.c （匹配3次）
	行  18: #include "php_hash.h"
	行  19: #include "php_hash_gost.h"
	行  20: #include "php_hash_gost_tables.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\hash_haval.c （匹配2次）
	行  17: #include "php_hash.h"
	行  18: #include "php_hash_haval.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\hash_joaat.c （匹配2次）
	行 21: #include "php_hash.h"
	行 22: #include "php_hash_joaat.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\hash_md.c （匹配2次）
	行  17: #include "php_hash.h"
	行  18: #include "php_hash_md.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\hash_murmur.c （匹配4次）
	行  17: #include "php_hash.h"
	行  18: #include "php_hash_murmur.h"
	行  20: #include "murmur/PMurHash.h"
	行  21: #include "murmur/PMurHash128.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\hash_ripemd.c （匹配2次）
	行  21: #include "php_hash.h"
	行  22: #include "php_hash_ripemd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\hash_sha.c （匹配2次）
	行  18: #include "php_hash.h"
	行  19: #include "php_hash_sha.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\hash_sha3.c （匹配3次）
	行  17: #include "php_hash.h"
	行  18: #include "php_hash_sha3.h"
	行 263: #include "KeccakHash.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\hash_snefru.c （匹配3次）
	行  18: #include "php_hash.h"
	行  19: #include "php_hash_snefru.h"
	行  20: #include "php_hash_snefru_tables.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\hash_tiger.c （匹配3次）
	行  18: #include "php_hash.h"
	行  19: #include "php_hash_tiger.h"
	行  20: #include "php_hash_tiger_tables.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\hash_whirlpool.c （匹配3次）
	行  18: #include "php_hash.h"
	行  25: #include "php_hash_whirlpool.h"
	行  26: #include "php_hash_whirlpool_tables.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\hash_xxhash.c （匹配2次）
	行  17: #include "php_hash.h"
	行  18: #include "php_hash_xxhash.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\murmur\\PMurHash.c （匹配3次）
	行  41: #include "PMurHash.h"
	行  73:   #include <stdlib.h>  /* Microsoft put _rotl declaration in here */
	行  81: #include "endianness.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\murmur\\PMurHash.h （匹配1次）
	行 23: #include <stdint.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\murmur\\PMurHash128.c （匹配3次）
	行  39: #include "PMurHash128.h"
	行  65:   #include <stdlib.h>  /* Microsoft put _rotl declaration in here */
	行  77: #include "endianness.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\murmur\\PMurHash128.h （匹配1次）
	行 23: #include <stdint.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\php_hash.h （匹配1次）
	行  20: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\php_hash_adler32.h （匹配1次）
	行 20: #include "ext/standard/basic_functions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\php_hash_crc32.h （匹配1次）
	行 20: #include "ext/standard/basic_functions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\php_hash_gost.h （匹配1次）
	行 20: #include "ext/standard/basic_functions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\php_hash_haval.h （匹配1次）
	行 20: #include "ext/standard/basic_functions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\php_hash_md.h （匹配1次）
	行 21: #include "ext/standard/md5.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\php_hash_ripemd.h （匹配1次）
	行 19: #include "ext/standard/basic_functions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\php_hash_sha.h （匹配2次）
	行 21: #include "ext/standard/sha1.h"
	行 22: #include "ext/standard/basic_functions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\php_hash_sha3.h （匹配1次）
	行 20: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\php_hash_snefru.h （匹配1次）
	行 24: #include "ext/standard/basic_functions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\php_hash_xxhash.h （匹配1次）
	行 21: #include "xxhash.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\sha3\\generic32lc\\KeccakHash.c （匹配2次）
	行 16: #include <string.h>
	行 17: #include "KeccakHash.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\sha3\\generic32lc\\KeccakHash.h （匹配2次）
	行  21: #include "KeccakSponge.h"
	行  22: #include <string.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\sha3\\generic32lc\\KeccakP-1600-inplace32BI.c （匹配4次）
	行   16: #include    <string.h>
	行   17: #include "brg_endian.h"
	行   18: #include "KeccakP-1600-SnP.h"
	行   19: #include "SnP-Relaned.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\sha3\\generic32lc\\KeccakSponge.c （匹配12次）
	行  16: #include "KeccakSponge.h"
	行  19:     #include "displayIntermediateValues.h"
	行  23:     #include "KeccakP-200-SnP.h"
	行  32:         #include "KeccakSponge.inc"
	行  41:     #include "KeccakP-400-SnP.h"
	行  50:         #include "KeccakSponge.inc"
	行  59:     #include "KeccakP-800-SnP.h"
	行  68:         #include "KeccakSponge.inc"
	行  77:     #include "KeccakP-1600-SnP.h"
	行  86:         #include "KeccakSponge.inc"
	行  95:     #include "KeccakP-1600-SnP.h"
	行 104:         #include "KeccakSponge.inc"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\sha3\\generic32lc\\KeccakSponge.h （匹配7次）
	行 130: #include <string.h>
	行 131: #include "align.h"
	行 149:     #include "KeccakP-200-SnP.h"
	行 155:     #include "KeccakP-400-SnP.h"
	行 161:     #include "KeccakP-800-SnP.h"
	行 167:     #include "KeccakP-1600-SnP.h"
	行 173:     #include "KeccakP-1600-SnP.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\sha3\\generic64lc\\KeccakHash.c （匹配2次）
	行 16: #include <string.h>
	行 17: #include "KeccakHash.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\sha3\\generic64lc\\KeccakHash.h （匹配2次）
	行  21: #include "KeccakSponge.h"
	行  22: #include <string.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\sha3\\generic64lc\\KeccakP-1600-opt64.c （匹配7次）
	行  16: #include <string.h>
	行  17: #include <stdlib.h>
	行  18: #include "brg_endian.h"
	行  19: #include "KeccakP-1600-opt64-config.h"
	行  49: #include "KeccakP-1600-64.macros"
	行  55: #include "KeccakP-1600-unrolling.macros"
	行  56: #include "SnP-Relaned.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\sha3\\generic64lc\\KeccakP-1600-SnP.h （匹配3次）
	行 22: #include "brg_endian.h"
	行 23: #include "KeccakP-1600-opt64-config.h"
	行 30: #include <stddef.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\sha3\\generic64lc\\KeccakSponge.c （匹配12次）
	行  16: #include "KeccakSponge.h"
	行  19:     #include "displayIntermediateValues.h"
	行  23:     #include "KeccakP-200-SnP.h"
	行  32:         #include "KeccakSponge.inc"
	行  41:     #include "KeccakP-400-SnP.h"
	行  50:         #include "KeccakSponge.inc"
	行  59:     #include "KeccakP-800-SnP.h"
	行  68:         #include "KeccakSponge.inc"
	行  77:     #include "KeccakP-1600-SnP.h"
	行  86:         #include "KeccakSponge.inc"
	行  95:     #include "KeccakP-1600-SnP.h"
	行 104:         #include "KeccakSponge.inc"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\sha3\\generic64lc\\KeccakSponge.h （匹配7次）
	行 130: #include <string.h>
	行 131: #include "align.h"
	行 149:     #include "KeccakP-200-SnP.h"
	行 155:     #include "KeccakP-400-SnP.h"
	行 161:     #include "KeccakP-800-SnP.h"
	行 167:     #include "KeccakP-1600-SnP.h"
	行 173:     #include "KeccakP-1600-SnP.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\hash\\xxhash\\xxhash.h （匹配9次）
	行   98:  *     #include "xxhash.h"
	行  342: #include <stddef.h>   /* size_t */
	行  441:  *    #include <stdio.h>
	行  442:  *    #include <xxhash.h>
	行 1240:  * which was then #included when inlining was activated.
	行 1460: #include <stdlib.h>
	行 1474: #include <string.h>
	行 1485: #include <limits.h>   /* ULLONG_MAX */
	行 5447: #include <string.h>   /* memcmp, memcpy */
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\iconv\\iconv.c （匹配16次）
	行   20: #include "config.h"
	行   23: #include "php.h"
	行   24: #include "php_globals.h"
	行   25: #include "ext/standard/info.h"
	行   26: #include "main/php_output.h"
	行   27: #include "SAPI.h"
	行   28: #include "php_ini.h"
	行   30: #include <stdlib.h>
	行   31: #include <errno.h>
	行   33: #include "php_iconv.h"
	行   37: #include <iconv.h>
	行   40: #include <gnu/libc-version.h>
	行   47: #include "zend_smart_str.h"
	行   48: #include "ext/standard/base64.h"
	行   49: #include "ext/standard/quot_print.h"
	行   76: #include "iconv_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\iconv\\php_iconv.h （匹配1次）
	行 33: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\imap\\php_imap.c （匹配20次）
	行   31: #include "config.h"
	行   34: #include "php.h"
	行   35: #include "php_ini.h"
	行   36: #include "php_streams.h"
	行   37: #include "Zend/zend_attributes.h"
	行   38: #include "Zend/zend_exceptions.h"
	行   39: #include "ext/standard/php_string.h"
	行   40: #include "ext/standard/info.h"
	行   41: #include "ext/standard/file.h"
	行   42: #include "zend_smart_str.h"
	行   43: #include "ext/pcre/php_pcre.h"
	行   48: #include "php_imap.h"
	行   50: #include <time.h>
	行   51: #include <stdio.h>
	行   52: #include <ctype.h>
	行   53: #include <signal.h>
	行   56: #include <winsock2.h>
	行   57: #include <stdlib.h>
	行   58: #include "win32/sendmail.h"
	行   77: #include "php_imap_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\imap\\php_imap.h （匹配1次）
	行  60: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\breakiterator\\breakiterator_class.h （匹配4次）
	行 19: #include <math.h>
	行 21: #include <php.h>
	行 22: #include "../intl_error.h"
	行 23: #include "../intl_data.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\breakiterator\\breakiterator_iterators.h （匹配3次）
	行 17: #include <unicode/umachine.h>
	行 20: #include <math.h>
	行 21: #include <php.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\breakiterator\\codepointiterator_internal.h （匹配2次）
	行  18: #include <unicode/brkiter.h>
	行  19: #include <unicode/unistr.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\calendar\\calendar_class.h （匹配4次）
	行 19: #include <math.h>
	行 21: #include <php.h>
	行 22: #include "intl_error.h"
	行 23: #include "intl_data.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\collator\\collator.h （匹配1次）
	行 19: #include <php.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\collator\\collator_attr.c （匹配5次）
	行  17: #include "config.h"
	行  20: #include "php_intl.h"
	行  21: #include "collator_class.h"
	行  22: #include "collator_convert.h"
	行  24: #include <unicode/ustring.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\collator\\collator_class.c （匹配8次）
	行  16: #include "collator.h"
	行  17: #include "collator_class.h"
	行  18: #include "php_intl.h"
	行  19: #include "collator_sort.h"
	行  20: #include "collator_convert.h"
	行  21: #include "intl_error.h"
	行  23: #include <unicode/ucol.h>
	行  25: #include "collator_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\collator\\collator_class.h （匹配5次）
	行 19: #include <php.h>
	行 21: #include "../intl_common.h"
	行 22: #include "../intl_error.h"
	行 23: #include "../intl_data.h"
	行 25: #include <unicode/ucol.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\collator\\collator_compare.c （匹配4次）
	行  17: #include "config.h"
	行  20: #include "php_intl.h"
	行  21: #include "collator_class.h"
	行  22: #include "intl_convert.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\collator\\collator_convert.c （匹配8次）
	行  17: #include "config.h"
	行  20: #include "php_intl.h"
	行  21: #include "collator_class.h"
	行  22: #include "collator_is_numeric.h"
	行  23: #include "collator_convert.h"
	行  24: #include "intl_convert.h"
	行  26: #include <unicode/ustring.h>
	行  27: #include <php.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\collator\\collator_convert.h （匹配2次）
	行 19: #include <php.h>
	行 20: #include <unicode/utypes.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\collator\\collator_create.c （匹配4次）
	行 17: #include "config.h"
	行 20: #include "php_intl.h"
	行 21: #include "collator_class.h"
	行 22: #include "intl_data.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\collator\\collator_error.c （匹配3次）
	行 17: #include "config.h"
	行 20: #include "php_intl.h"
	行 21: #include "collator_class.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\collator\\collator_is_numeric.c （匹配1次）
	行  16: #include "collator_is_numeric.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\collator\\collator_is_numeric.h （匹配2次）
	行 19: #include <php.h>
	行 20: #include <unicode/uchar.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\collator\\collator_locale.c （匹配5次）
	行 17: #include "config.h"
	行 20: #include "php_intl.h"
	行 21: #include "collator_class.h"
	行 22: #include "intl_convert.h"
	行 24: #include <zend_API.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\collator\\collator_sort.c （匹配7次）
	行  17: #include "config.h"
	行  20: #include "php_intl.h"
	行  21: #include "collator.h"
	行  22: #include "collator_class.h"
	行  23: #include "collator_sort.h"
	行  24: #include "collator_convert.h"
	行  25: #include "intl_convert.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\collator\\collator_sort.h （匹配1次）
	行 19: #include <php.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\common\\common_date.h （匹配4次）
	行 18: #include <unicode/umachine.h>
	行 21: #include <php.h>
	行 22: #include "../intl_error.h"
	行 27: #include <unicode/timezone.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\common\\common_enum.h （匹配6次）
	行 19: #include <unicode/umachine.h>
	行 21: #include <unicode/strenum.h>
	行 23: #include <math.h>
	行 25: #include <php.h>
	行 26: #include "../intl_error.h"
	行 27: #include "../intl_data.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\common\\common_error.c （匹配3次）
	行 17: #include "config.h"
	行 20: #include "php_intl.h"
	行 21: #include "intl_error.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\converter\\converter.c （匹配10次）
	行  15: #include "converter.h"
	行  16: #include "zend_exceptions.h"
	行  18: #include <unicode/utypes.h>
	行  19: #include <unicode/utf8.h>
	行  20: #include <unicode/utf16.h>
	行  21: #include <unicode/ucnv.h>
	行  22: #include <unicode/ustring.h>
	行  24: #include "../intl_error.h"
	行  25: #include "../intl_common.h"
	行  26: #include "converter_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\converter\\converter.h （匹配2次）
	行 19: #include "config.h"
	行 22: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\dateformat\\dateformat.c （匹配5次）
	行 15: #include "config.h"
	行 18: #include <unicode/udat.h>
	行 20: #include "php_intl.h"
	行 21: #include "dateformat_class.h"
	行 22: #include "dateformat.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\dateformat\\dateformat.h （匹配1次）
	行 17: #include <php.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\dateformat\\dateformat_attr.c （匹配7次）
	行  15: #include "config.h"
	行  18: #include "../php_intl.h"
	行  19: #include "dateformat_class.h"
	行  20: #include "../intl_convert.h"
	行  21: #include "dateformat_class.h"
	行  23: #include <unicode/ustring.h>
	行  24: #include <unicode/udat.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\dateformat\\dateformat_class.c （匹配7次）
	行  14: #include <unicode/unum.h>
	行  16: #include "dateformat_class.h"
	行  17: #include "php_intl.h"
	行  18: #include "dateformat_data.h"
	行  19: #include "dateformat.h"
	行  20: #include "dateformat_arginfo.h"
	行  22: #include <zend_exceptions.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\dateformat\\dateformat_class.h （匹配5次）
	行 17: #include <php.h>
	行 19: #include "intl_common.h"
	行 20: #include "intl_error.h"
	行 21: #include "intl_data.h"
	行 22: #include "dateformat_data.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\dateformat\\dateformat_create.h （匹配1次）
	行 17: #include <php.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\dateformat\\dateformat_data.c （匹配2次）
	行 15: #include "config.h"
	行 18: #include "dateformat_data.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\dateformat\\dateformat_data.h （匹配3次）
	行 17: #include <php.h>
	行 19: #include <unicode/udat.h>
	行 21: #include "intl_error.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\dateformat\\dateformat_format.c （匹配9次）
	行  16: #include "config.h"
	行  19: #include "../php_intl.h"
	行  21: #include <unicode/ustring.h>
	行  22: #include <unicode/ucal.h>
	行  24: #include "../intl_convert.h"
	行  25: #include "../common/common_date.h"
	行  26: #include "dateformat.h"
	行  27: #include "dateformat_class.h"
	行  28: #include "dateformat_data.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\dateformat\\dateformat_helpers.h （匹配3次）
	行 22: #include <unicode/calendar.h>
	行 23: #include <unicode/datefmt.h>
	行 26: #include "../php_intl.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\dateformat\\dateformat_parse.c （匹配8次）
	行  16: #include "config.h"
	行  19: #include <unicode/ustring.h>
	行  20: #include <math.h>
	行  22: #include "php_intl.h"
	行  23: #include "intl_convert.h"
	行  24: #include "dateformat.h"
	行  25: #include "dateformat_class.h"
	行  26: #include "dateformat_data.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\dateformat\\datepatterngenerator_class.h （匹配4次）
	行 18: #include <php.h>
	行 19: #include "intl_common.h"
	行 20: #include "intl_error.h"
	行 21: #include "intl_data.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\formatter\\formatter_attr.c （匹配5次）
	行  16: #include "config.h"
	行  19: #include "php_intl.h"
	行  20: #include "formatter_class.h"
	行  21: #include "intl_convert.h"
	行  23: #include <unicode/ustring.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\formatter\\formatter_class.c （匹配8次）
	行  15: #include <unicode/unum.h>
	行  17: #include "formatter_class.h"
	行  18: #include "php_intl.h"
	行  19: #include "formatter_data.h"
	行  20: #include "formatter_format.h"
	行  21: #include "formatter_arginfo.h"
	行  23: #include <zend_exceptions.h>
	行  24: #include "Zend/zend_interfaces.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\formatter\\formatter_class.h （匹配5次）
	行 18: #include <php.h>
	行 20: #include "intl_common.h"
	行 21: #include "intl_error.h"
	行 22: #include "intl_data.h"
	行 23: #include "formatter_data.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\formatter\\formatter_data.c （匹配2次）
	行 16: #include "config.h"
	行 19: #include "formatter_data.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\formatter\\formatter_data.h （匹配3次）
	行 18: #include <php.h>
	行 20: #include <unicode/unum.h>
	行 22: #include "intl_error.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\formatter\\formatter_format.c （匹配6次）
	行  16: #include "config.h"
	行  19: #include "php_intl.h"
	行  21: #include <unicode/ustring.h>
	行  23: #include "formatter_class.h"
	行  24: #include "formatter_format.h"
	行  25: #include "intl_convert.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\formatter\\formatter_format.h （匹配1次）
	行 18: #include <php.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\formatter\\formatter_main.c （匹配5次）
	行  16: #include "config.h"
	行  19: #include <unicode/ustring.h>
	行  21: #include "php_intl.h"
	行  22: #include "formatter_class.h"
	行  23: #include "intl_convert.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\formatter\\formatter_parse.c （匹配7次）
	行  16: #include "config.h"
	行  19: #include "php_intl.h"
	行  21: #include <unicode/ustring.h>
	行  22: #include <locale.h>
	行  24: #include "formatter_class.h"
	行  25: #include "formatter_format.h"
	行  26: #include "intl_convert.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\grapheme\\grapheme.h （匹配2次）
	行 18: #include <php.h>
	行 19: #include <unicode/utypes.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\grapheme\\grapheme_string.c （匹配9次）
	行  17: #include "config.h"
	行  20: #include <php.h>
	行  21: #include "grapheme.h"
	行  22: #include "grapheme_util.h"
	行  24: #include <unicode/utypes.h>
	行  25: #include <unicode/utf8.h>
	行  26: #include <unicode/ucol.h>
	行  27: #include <unicode/ustring.h>
	行  28: #include <unicode/ubrk.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\grapheme\\grapheme_util.c （匹配11次）
	行  17: #include "config.h"
	行  20: #include <php.h>
	行  21: #include "grapheme.h"
	行  22: #include "grapheme_util.h"
	行  23: #include "intl_common.h"
	行  25: #include <unicode/utypes.h>
	行  26: #include <unicode/ucol.h>
	行  27: #include <unicode/ustring.h>
	行  28: #include <unicode/ubrk.h>
	行  29: #include <unicode/usearch.h>
	行  31: #include "ext/standard/php_string.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\grapheme\\grapheme_util.h （匹配2次）
	行 18: #include "php_intl.h"
	行 19: #include "intl_convert.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\idn\\idn.c （匹配8次）
	行  20: #include "config.h"
	行  23: #include <php.h>
	行  25: #include <unicode/uidna.h>
	行  26: #include <unicode/ustring.h>
	行  27: #include "ext/standard/php_string.h"
	行  29: #include "idn.h"
	行  30: #include "intl_error.h"
	行  31: #include "intl_convert.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\idn\\idn.h （匹配1次）
	行 20: #include <php.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\intl_common.h （匹配2次）
	行 22: #include <php.h>
	行 24: #include <unicode/utypes.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\intl_convert.c （匹配4次）
	行  17: #include "config.h"
	行  20: #include <php.h>
	行  22: #include "intl_common.h"
	行  23: #include "intl_convert.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\intl_convert.h （匹配1次）
	行 19: #include <unicode/ustring.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\intl_convertcpp.h （匹配2次）
	行 22: #include <unicode/unistr.h>
	行 23: #include <zend_types.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\intl_cppshims.h （匹配2次）
	行 24: #include <stdio.h>
	行 26: #include <math.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\intl_data.h （匹配2次）
	行  20: #include <unicode/utypes.h>
	行  22: #include "intl_error.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\intl_error.c （匹配6次）
	行  18: #include "config.h"
	行  21: #include <php.h>
	行  22: #include <zend_exceptions.h>
	行  24: #include "php_intl.h"
	行  25: #include "intl_error.h"
	行  26: #include "intl_convert.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\intl_error.h （匹配3次）
	行 20: #include <unicode/utypes.h>
	行 21: #include <unicode/parseerr.h>
	行 22: #include <zend_smart_str.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\locale\\locale.c （匹配6次）
	行 16: #include "config.h"
	行 19: #include "locale_class.h"
	行 20: #include "locale.h"
	行 22: #include <unicode/utypes.h>
	行 23: #include <unicode/uloc.h>
	行 24: #include <unicode/ustring.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\locale\\locale.h （匹配1次）
	行 18: #include <php.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\locale\\locale_class.c （匹配6次）
	行 15: #include <unicode/uloc.h>
	行 16: #include "php_intl.h"
	行 17: #include "intl_error.h"
	行 18: #include "locale_class.h"
	行 19: #include "locale.h"
	行 20: #include "locale_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\locale\\locale_class.h （匹配4次）
	行 18: #include <php.h>
	行 20: #include "intl_common.h"
	行 21: #include "intl_error.h"
	行 23: #include <unicode/uloc.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\locale\\locale_methods.c （匹配15次）
	行   16: #include "config.h"
	行   19: #include <unicode/ustring.h>
	行   20: #include <unicode/udata.h>
	行   21: #include <unicode/putil.h>
	行   22: #include <unicode/ures.h>
	行   24: #include "php_intl.h"
	行   25: #include "locale.h"
	行   26: #include "locale_class.h"
	行   27: #include "intl_convert.h"
	行   28: #include "intl_data.h"
	行   30: #include <zend_API.h>
	行   31: #include <zend.h>
	行   32: #include <php.h>
	行   33: #include "main/php_ini.h"
	行   34: #include "zend_smart_str.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\msgformat\\msgformat.c （匹配7次）
	行  16: #include "config.h"
	行  19: #include <unicode/ustring.h>
	行  20: #include <unicode/umsg.h>
	行  22: #include "php_intl.h"
	行  23: #include "msgformat_class.h"
	行  24: #include "msgformat_data.h"
	行  25: #include "intl_convert.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\msgformat\\msgformat_attr.c （匹配6次）
	行  16: #include "config.h"
	行  19: #include "php_intl.h"
	行  20: #include "msgformat_class.h"
	行  21: #include "msgformat_data.h"
	行  22: #include "intl_convert.h"
	行  24: #include <unicode/ustring.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\msgformat\\msgformat_class.c （匹配6次）
	行  15: #include <unicode/unum.h>
	行  17: #include "msgformat_class.h"
	行  18: #include "php_intl.h"
	行  19: #include "msgformat_data.h"
	行  20: #include "msgformat_arginfo.h"
	行  22: #include <zend_exceptions.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\msgformat\\msgformat_class.h （匹配6次）
	行 18: #include <php.h>
	行 20: #include <unicode/uconfig.h>
	行 22: #include "../intl_common.h"
	行 23: #include "../intl_error.h"
	行 24: #include "../intl_data.h"
	行 26: #include "msgformat_data.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\msgformat\\msgformat_data.c （匹配4次）
	行 16: #include "config.h"
	行 19: #include <unicode/ustring.h>
	行 20: #include "msgformat_data.h"
	行 22: #include "msgformat_class.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\msgformat\\msgformat_data.h （匹配3次）
	行 18: #include <php.h>
	行 20: #include "../intl_error.h"
	行 22: #include <unicode/umsg.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\msgformat\\msgformat_format.c （匹配7次）
	行  16: #include "config.h"
	行  19: #include <unicode/ustring.h>
	行  21: #include "php_intl.h"
	行  22: #include "msgformat_class.h"
	行  23: #include "msgformat_data.h"
	行  24: #include "msgformat_helpers.h"
	行  25: #include "intl_convert.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\msgformat\\msgformat_parse.c （匹配7次）
	行  16: #include "config.h"
	行  19: #include <unicode/ustring.h>
	行  21: #include "php_intl.h"
	行  22: #include "msgformat_class.h"
	行  23: #include "msgformat_data.h"
	行  24: #include "msgformat_helpers.h"
	行  25: #include "intl_convert.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\normalizer\\normalizer.h （匹配4次）
	行 18: #include <php.h>
	行 19: #include <unicode/utypes.h>
	行 21: #include <unicode/unorm.h>
	行 33: #include <unicode/unorm2.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\normalizer\\normalizer_class.c （匹配6次）
	行 15: #include "normalizer.h"
	行 16: #include "normalizer_class.h"
	行 17: #include "php_intl.h"
	行 18: #include "normalizer_arginfo.h"
	行 19: #include "intl_error.h"
	行 21: #include <unicode/unorm.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\normalizer\\normalizer_class.h （匹配4次）
	行 18: #include <php.h>
	行 20: #include "intl_common.h"
	行 21: #include "intl_error.h"
	行 23: #include <unicode/unorm.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\normalizer\\normalizer_normalize.c （匹配8次）
	行  16: #include "config.h"
	行  19: #include "php_intl.h"
	行  21: #include "unicode/unorm.h"
	行  23: #include <unicode/unorm2.h>
	行  25: #include "normalizer.h"
	行  26: #include "normalizer_class.h"
	行  27: #include "intl_convert.h"
	行  28: #include <unicode/utf8.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\php_intl.c （匹配35次）
	行  19: #include "config.h"
	行  23: #include "php_intl.h"
	行  24: #include "intl_error.h"
	行  25: #include "collator/collator_class.h"
	行  26: #include "collator/collator.h"
	行  27: #include "collator/collator_sort.h"
	行  28: #include "collator/collator_convert.h"
	行  30: #include "converter/converter.h"
	行  32: #include "formatter/formatter_class.h"
	行  33: #include "formatter/formatter_format.h"
	行  35: #include "grapheme/grapheme.h"
	行  37: #include "msgformat/msgformat_class.h"
	行  39: #include "normalizer/normalizer_class.h"
	行  41: #include "locale/locale.h"
	行  42: #include "locale/locale_class.h"
	行  44: #include "dateformat/dateformat.h"
	行  45: #include "dateformat/dateformat_class.h"
	行  46: #include "dateformat/dateformat_data.h"
	行  47: #include "dateformat/datepatterngenerator_class.h"
	行  49: #include "resourcebundle/resourcebundle_class.h"
	行  51: #include "transliterator/transliterator.h"
	行  52: #include "transliterator/transliterator_class.h"
	行  54: #include "timezone/timezone_class.h"
	行  56: #include "calendar/calendar_class.h"
	行  58: #include "breakiterator/breakiterator_class.h"
	行  59: #include "breakiterator/breakiterator_iterators.h"
	行  61: #include <unicode/uidna.h>
	行  62: #include "idn/idn.h"
	行  63: #include "uchar/uchar.h"
	行  67: #include "common/common_enum.h"
	行  69: #include <unicode/uloc.h>
	行  70: #include <unicode/uclean.h>
	行  71: #include <ext/standard/info.h>
	行  73: #include "php_ini.h"
	行  75: #include "php_intl_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\php_intl.h （匹配6次）
	行 21: #include <php.h>
	行 28: #include "collator/collator_sort.h"
	行 29: #include <unicode/ubrk.h>
	行 30: #include "intl_error.h"
	行 31: #include "Zend/zend_exceptions.h"
	行 43: #include "TSRM.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\resourcebundle\\resourcebundle.c （匹配6次）
	行 15: #include <unicode/ures.h>
	行 17: #include <zend.h>
	行 18: #include <zend_API.h>
	行 20: #include "intl_convert.h"
	行 21: #include "intl_data.h"
	行 22: #include "resourcebundle/resourcebundle_class.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\resourcebundle\\resourcebundle.h （匹配3次）
	行 18: #include <unicode/ures.h>
	行 20: #include <zend.h>
	行 22: #include "resourcebundle/resourcebundle_class.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\resourcebundle\\resourcebundle_class.c （匹配14次）
	行  15: #include <stdlib.h>
	行  16: #include <unicode/ures.h>
	行  17: #include <unicode/uenum.h>
	行  19: #include <zend.h>
	行  20: #include <Zend/zend_exceptions.h>
	行  21: #include <Zend/zend_interfaces.h>
	行  22: #include <php.h>
	行  24: #include "php_intl.h"
	行  25: #include "intl_data.h"
	行  26: #include "intl_common.h"
	行  28: #include "resourcebundle/resourcebundle.h"
	行  29: #include "resourcebundle/resourcebundle_iterator.h"
	行  30: #include "resourcebundle/resourcebundle_class.h"
	行  31: #include "resourcebundle/resourcebundle_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\resourcebundle\\resourcebundle_class.h （匹配4次）
	行 18: #include <unicode/ures.h>
	行 20: #include <zend.h>
	行 21: #include "php.h"
	行 23: #include "intl_error.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\resourcebundle\\resourcebundle_iterator.c （匹配6次）
	行  15: #include <php.h>
	行  16: #include <zend.h>
	行  17: #include <zend_API.h>
	行  19: #include "resourcebundle/resourcebundle.h"
	行  20: #include "resourcebundle/resourcebundle_class.h"
	行  21: #include "resourcebundle/resourcebundle_iterator.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\resourcebundle\\resourcebundle_iterator.h （匹配2次）
	行 18: #include <zend.h>
	行 20: #include "resourcebundle/resourcebundle_class.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\spoofchecker\\spoofchecker_class.c （匹配5次）
	行  15: #include "spoofchecker_class.h"
	行  16: #include "spoofchecker_arginfo.h"
	行  17: #include "php_intl.h"
	行  18: #include "intl_error.h"
	行  20: #include <unicode/uspoof.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\spoofchecker\\spoofchecker_class.h （匹配5次）
	行 18: #include <php.h>
	行 20: #include "intl_common.h"
	行 21: #include "intl_error.h"
	行 22: #include "intl_data.h"
	行 24: #include <unicode/uspoof.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\spoofchecker\\spoofchecker_create.c （匹配4次）
	行 16: #include "config.h"
	行 19: #include "php_intl.h"
	行 20: #include "spoofchecker_class.h"
	行 21: #include "intl_data.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\spoofchecker\\spoofchecker_main.c （匹配3次）
	行  16: #include "config.h"
	行  19: #include "php_intl.h"
	行  20: #include "spoofchecker_class.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\timezone\\timezone_class.h （匹配5次）
	行 19: #include <math.h>
	行 22: #include <stdio.h>
	行 24: #include <php.h>
	行 25: #include "intl_error.h"
	行 26: #include "intl_data.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\transliterator\\transliterator.h （匹配3次）
	行 18: #include <php.h>
	行 19: #include <unicode/utrans.h>
	行 21: #include "zend_smart_str.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\transliterator\\transliterator_class.c （匹配7次）
	行  15: #include "transliterator_class.h"
	行  16: #include "php_intl.h"
	行  17: #include "transliterator_arginfo.h"
	行  18: #include "intl_error.h"
	行  19: #include "intl_convert.h"
	行  20: #include "intl_data.h"
	行  22: #include <unicode/utrans.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\transliterator\\transliterator_class.h （匹配4次）
	行 18: #include <php.h>
	行 20: #include "intl_common.h"
	行 21: #include "intl_error.h"
	行 23: #include <unicode/utrans.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\transliterator\\transliterator_methods.c （匹配7次）
	行  16: #include "config.h"
	行  19: #include "php_intl.h"
	行  20: #include "transliterator.h"
	行  21: #include "transliterator_class.h"
	行  22: #include "intl_data.h"
	行  23: #include "intl_convert.h"
	行  25: #include <zend_exceptions.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\uchar\\uchar.c （匹配6次）
	行   1: #include "uchar.h"
	行   2: #include "intl_data.h"
	行   3: #include "intl_convert.h"
	行   5: #include <unicode/uchar.h>
	行   6: #include <unicode/utf8.h>
	行   8: #include "uchar_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\intl\\uchar\\uchar.h （匹配1次）
	行 4: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\json\\json.c （匹配11次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  23: #include "php_ini.h"
	行  24: #include "ext/standard/info.h"
	行  25: #include "ext/standard/html.h"
	行  26: #include "zend_smart_str.h"
	行  27: #include "php_json.h"
	行  28: #include "php_json_encoder.h"
	行  29: #include "php_json_parser.h"
	行  30: #include "json_arginfo.h"
	行  31: #include <zend_exceptions.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\json\\json_encoder.c （匹配10次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  23: #include "php_ini.h"
	行  24: #include "ext/standard/info.h"
	行  25: #include "ext/standard/html.h"
	行  26: #include "zend_smart_str.h"
	行  27: #include "php_json.h"
	行  28: #include "php_json_encoder.h"
	行  29: #include <zend_exceptions.h>
	行  30: #include "zend_enum.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\json\\php_json.h （匹配3次）
	行  21: #include "php_version.h"
	行  22: #include "zend_smart_str_public.h"
	行  36: #include "TSRM.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\json\\php_json_encoder.h （匹配2次）
	行 20: #include "php.h"
	行 21: #include "zend_smart_str.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\json\\php_json_parser.h （匹配2次）
	行 20: #include "php.h"
	行 21: #include "php_json_scanner.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\json\\php_json_scanner.h （匹配2次）
	行 20: #include "php.h"
	行 21: #include "php_json.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\ldap\\ldap.c （匹配12次）
	行   26: #include "config.h"
	行   29: #include "php.h"
	行   30: #include "php_ini.h"
	行   31: #include "Zend/zend_attributes.h"
	行   33: #include <stddef.h>
	行   35: #include "ext/standard/dl.h"
	行   36: #include "php_ldap.h"
	行   39: #include <string.h>
	行   40: #include "config.w32.h"
	行   48: #include "ext/standard/info.h"
	行   51: #include <sasl/sasl.h>
	行   57: #include "ldap_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\ldap\\php_ldap.h （匹配3次）
	行 23: #include <lber.h>
	行 26: #include <ldap.h>
	行 31: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\libxml\\libxml.c （匹配17次）
	行   19: #include "config.h"
	行   22: #include "php.h"
	行   23: #include "SAPI.h"
	行   25: #include "zend_variables.h"
	行   26: #include "ext/standard/php_string.h"
	行   27: #include "ext/standard/info.h"
	行   28: #include "ext/standard/file.h"
	行   32: #include <libxml/parser.h>
	行   33: #include <libxml/parserInternals.h>
	行   34: #include <libxml/tree.h>
	行   35: #include <libxml/uri.h>
	行   36: #include <libxml/xmlerror.h>
	行   37: #include <libxml/xmlsave.h>
	行   39: #include <libxml/relaxng.h>
	行   40: #include <libxml/xmlschemas.h>
	行   43: #include "php_libxml.h"
	行   50: #include "libxml_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\libxml\\php_libxml.h （匹配3次）
	行  25: #include "php_version.h"
	行  36: #include "zend_smart_str.h"
	行  37: #include <libxml/tree.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\html_entities.c （匹配2次）
	行  30: #include "mbfilter.h"
	行  31: #include "html_entities.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_7bit.c （匹配2次）
	行  31: #include "mbfilter.h"
	行  32: #include "mbfilter_7bit.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_7bit.h （匹配1次）
	行 34: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_base64.c （匹配2次）
	行  31: #include "mbfilter.h"
	行  32: #include "mbfilter_base64.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_base64.h （匹配1次）
	行 34: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_big5.c （匹配3次）
	行  30: #include "mbfilter.h"
	行  31: #include "mbfilter_big5.h"
	行  33: #include "unicode_table_big5.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_big5.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_cp5022x.c （匹配7次）
	行   25: #include "mbfilter.h"
	行   26: #include "mbfilter_cp5022x.h"
	行   27: #include "mbfilter_jis.h"
	行   29: #include "unicode_table_cp932_ext.h"
	行   30: #include "unicode_table_jis.h"
	行   31: #include "cp932_table.h"
	行   32: #include "translit_kana_jisx0201_jisx0208.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_cp5022x.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_cp51932.c （匹配5次）
	行  30: #include "mbfilter.h"
	行  31: #include "mbfilter_cp51932.h"
	行  33: #include "unicode_table_cp932_ext.h"
	行  34: #include "unicode_table_jis.h"
	行  35: #include "cp932_table.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_cp51932.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_cp932.c （匹配4次）
	行  60: #include "mbfilter.h"
	行  61: #include "mbfilter_cp932.h"
	行  63: #include "unicode_table_cp932_ext.h"
	行  64: #include "unicode_table_jis.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_cp932.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_cp936.c （匹配3次）
	行  30: #include "mbfilter.h"
	行  31: #include "mbfilter_cp936.h"
	行  33: #include "unicode_table_cp936.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_cp936.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_euc_cn.c （匹配3次）
	行  30: #include "mbfilter.h"
	行  31: #include "mbfilter_euc_cn.h"
	行  33: #include "unicode_table_cp936.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_euc_cn.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_euc_jp.c （匹配4次）
	行  30: #include "mbfilter.h"
	行  31: #include "mbfilter_euc_jp.h"
	行  33: #include "unicode_table_cp932_ext.h"
	行  34: #include "unicode_table_jis.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_euc_jp.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_euc_jp_2004.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_euc_jp_win.c （匹配5次）
	行  30: #include "mbfilter.h"
	行  31: #include "mbfilter_euc_jp_win.h"
	行  33: #include "unicode_table_cp932_ext.h"
	行  34: #include "unicode_table_jis.h"
	行  35: #include "cp932_table.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_euc_jp_win.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_euc_kr.c （匹配3次）
	行  30: #include "mbfilter.h"
	行  31: #include "mbfilter_euc_kr.h"
	行  32: #include "unicode_table_uhc.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_euc_kr.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_euc_tw.c （匹配3次）
	行  30: #include "mbfilter.h"
	行  31: #include "mbfilter_euc_tw.h"
	行  33: #include "unicode_table_cns11643.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_euc_tw.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_gb18030.c （匹配4次）
	行  30: #include "mbfilter.h"
	行  31: #include "mbfilter_gb18030.h"
	行  33: #include "unicode_table_cp936.h"
	行  34: #include "unicode_table_gb18030.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_gb18030.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_htmlent.c （匹配4次）
	行  30: #include <string.h>
	行  31: #include "mbfilter.h"
	行  32: #include "mbfilter_htmlent.h"
	行  33: #include "html_entities.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_htmlent.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_hz.c （匹配4次）
	行  30: #include "mbfilter.h"
	行  31: #include "mbfilter_hz.h"
	行  33: #include "unicode_table_cp936.h"
	行  34: #include "unicode_table_gb2312.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_hz.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_iso2022jp_2004.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_iso2022jp_mobile.c （匹配6次）
	行  30: #include "mbfilter.h"
	行  31: #include "mbfilter_iso2022jp_mobile.h"
	行  33: #include "unicode_table_cp932_ext.h"
	行  34: #include "unicode_table_jis.h"
	行  35: #include "cp932_table.h"
	行  36: #include "emoji2uni.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_iso2022jp_mobile.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_iso2022_jp_ms.c （匹配5次）
	行  30: #include "mbfilter.h"
	行  31: #include "mbfilter_iso2022_jp_ms.h"
	行  33: #include "unicode_table_cp932_ext.h"
	行  34: #include "unicode_table_jis.h"
	行  35: #include "cp932_table.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_iso2022_jp_ms.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_iso2022_kr.c （匹配3次）
	行  38: #include "mbfilter.h"
	行  39: #include "mbfilter_iso2022_kr.h"
	行  40: #include "unicode_table_uhc.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_iso2022_kr.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_jis.c （匹配4次）
	行  30: #include "mbfilter.h"
	行  31: #include "mbfilter_jis.h"
	行  33: #include "unicode_table_cp932_ext.h"
	行  34: #include "unicode_table_jis.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_jis.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_qprint.c （匹配3次）
	行  30: #include "mbfilter.h"
	行  31: #include "mbfilter_qprint.h"
	行  32: #include "unicode_prop.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_qprint.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_singlebyte.c （匹配1次）
	行  15: #include "mbfilter_singlebyte.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_singlebyte.h （匹配1次）
	行 18: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_sjis.c （匹配4次）
	行  30: #include "mbfilter.h"
	行  31: #include "mbfilter_sjis.h"
	行  36: #include "unicode_table_cp932_ext.h"
	行  37: #include "unicode_table_jis.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_sjis.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_sjis_2004.c （匹配6次）
	行   36: #include "mbfilter.h"
	行   37: #include "mbfilter_sjis_2004.h"
	行   38: #include "mbfilter_euc_jp_2004.h"
	行   39: #include "mbfilter_iso2022jp_2004.h"
	行   41: #include "unicode_table_jis2004.h"
	行   42: #include "unicode_table_jis.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_sjis_2004.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_sjis_mac.c （匹配5次）
	行   30: #include "mbfilter.h"
	行   31: #include "mbfilter_sjis_mac.h"
	行   33: #include "unicode_table_cp932_ext.h"
	行   34: #include "unicode_table_jis.h"
	行   36: #include "sjis_mac2uni.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_sjis_mac.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_sjis_mobile.c （匹配5次）
	行   30: #include "mbfilter.h"
	行   31: #include "mbfilter_sjis_mobile.h"
	行   33: #include "unicode_table_cp932_ext.h"
	行   34: #include "unicode_table_jis.h"
	行   36: #include "emoji2uni.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_sjis_mobile.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_ucs2.c （匹配2次）
	行  30: #include "mbfilter.h"
	行  31: #include "mbfilter_ucs2.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_ucs2.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_ucs4.c （匹配2次）
	行  30: #include "mbfilter.h"
	行  31: #include "mbfilter_ucs4.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_uhc.c （匹配3次）
	行  34: #include "mbfilter.h"
	行  35: #include "mbfilter_uhc.h"
	行  37: #include "unicode_table_uhc.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_uhc.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_utf16.c （匹配2次）
	行  30: #include "mbfilter.h"
	行  31: #include "mbfilter_utf16.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_utf32.c （匹配2次）
	行  30: #include "mbfilter.h"
	行  31: #include "mbfilter_utf32.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_utf7.c （匹配3次）
	行  30: #include "mbfilter.h"
	行  31: #include "mbfilter_utf7.h"
	行  32: #include "utf7_helper.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_utf7.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_utf7imap.c （匹配3次）
	行  78: #include "mbfilter.h"
	行  79: #include "mbfilter_utf7imap.h"
	行  80: #include "utf7_helper.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_utf7imap.h （匹配1次）
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_utf8.c （匹配2次）
	行  30: #include "mbfilter.h"
	行  31: #include "mbfilter_utf8.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_utf8_mobile.c （匹配4次）
	行  30: #include "mbfilter.h"
	行  32: #include "mbfilter_utf8_mobile.h"
	行  33: #include "mbfilter_sjis_mobile.h"
	行  35: #include "emoji2uni.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\mbfilter_uuencode.c （匹配2次）
	行  30: #include "mbfilter.h"
	行  31: #include "mbfilter_uuencode.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\filters\\utf7_helper.h （匹配1次）
	行  4: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\mbfl\\mbfilter.c （匹配13次）
	行   82: #include <stddef.h>
	行   83: #include <string.h>
	行   85: #include "mbfilter.h"
	行   86: #include "mbfl_filter_output.h"
	行   87: #include "mbfilter_8bit.h"
	行   88: #include "mbfilter_wchar.h"
	行   89: #include "mbstring.h"
	行   90: #include "php_unicode.h"
	行   91: #include "filters/mbfilter_base64.h"
	行   92: #include "filters/mbfilter_qprint.h"
	行   93: #include "filters/mbfilter_singlebyte.h"
	行   94: #include "filters/mbfilter_utf8.h"
	行   96: #include "rare_cp_bitvec.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\mbfl\\mbfilter.h （匹配7次）
	行  88: #include "zend.h"
	行  90: #include "mbfl_defs.h"
	行  91: #include "mbfl_consts.h"
	行  92: #include "mbfl_encoding.h"
	行  93: #include "mbfl_language.h"
	行  94: #include "mbfl_string.h"
	行  95: #include "mbfl_convert.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\mbfl\\mbfilter_8bit.c （匹配2次）
	行  31: #include <stddef.h>
	行  33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\mbfl\\mbfilter_8bit.h （匹配2次）
	行 34: #include "mbfl_defs.h"
	行 35: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\mbfl\\mbfilter_pass.c （匹配3次）
	行 30: #include <stddef.h>
	行 32: #include "mbfilter.h"
	行 33: #include "mbfilter_pass.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\mbfl\\mbfilter_pass.h （匹配2次）
	行 33: #include "mbfl_defs.h"
	行 34: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\mbfl\\mbfilter_wchar.c （匹配2次）
	行 31: #include <stddef.h>
	行 33: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\mbfl\\mbfilter_wchar.h （匹配2次）
	行 34: #include "mbfl_defs.h"
	行 35: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\mbfl\\mbfl_convert.c （匹配43次）
	行  31: #include <stddef.h>
	行  33: #include "mbfl_encoding.h"
	行  34: #include "mbfl_filter_output.h"
	行  35: #include "mbfilter_pass.h"
	行  36: #include "mbfilter_8bit.h"
	行  37: #include "mbfilter_wchar.h"
	行  39: #include "filters/mbfilter_euc_cn.h"
	行  40: #include "filters/mbfilter_hz.h"
	行  41: #include "filters/mbfilter_euc_tw.h"
	行  42: #include "filters/mbfilter_big5.h"
	行  43: #include "filters/mbfilter_uhc.h"
	行  44: #include "filters/mbfilter_euc_kr.h"
	行  45: #include "filters/mbfilter_iso2022_kr.h"
	行  46: #include "filters/mbfilter_sjis.h"
	行  47: #include "filters/mbfilter_sjis_2004.h"
	行  48: #include "filters/mbfilter_sjis_mobile.h"
	行  49: #include "filters/mbfilter_sjis_mac.h"
	行  50: #include "filters/mbfilter_cp51932.h"
	行  51: #include "filters/mbfilter_jis.h"
	行  52: #include "filters/mbfilter_iso2022_jp_ms.h"
	行  53: #include "filters/mbfilter_iso2022jp_2004.h"
	行  54: #include "filters/mbfilter_iso2022jp_mobile.h"
	行  55: #include "filters/mbfilter_euc_jp.h"
	行  56: #include "filters/mbfilter_euc_jp_2004.h"
	行  57: #include "filters/mbfilter_euc_jp_win.h"
	行  58: #include "filters/mbfilter_gb18030.h"
	行  59: #include "filters/mbfilter_cp932.h"
	行  60: #include "filters/mbfilter_cp936.h"
	行  61: #include "filters/mbfilter_cp5022x.h"
	行  62: #include "filters/mbfilter_base64.h"
	行  63: #include "filters/mbfilter_qprint.h"
	行  64: #include "filters/mbfilter_uuencode.h"
	行  65: #include "filters/mbfilter_7bit.h"
	行  66: #include "filters/mbfilter_utf7.h"
	行  67: #include "filters/mbfilter_utf7imap.h"
	行  68: #include "filters/mbfilter_utf8.h"
	行  69: #include "filters/mbfilter_utf8_mobile.h"
	行  70: #include "filters/mbfilter_utf16.h"
	行  71: #include "filters/mbfilter_utf32.h"
	行  72: #include "filters/mbfilter_ucs4.h"
	行  73: #include "filters/mbfilter_ucs2.h"
	行  74: #include "filters/mbfilter_htmlent.h"
	行  75: #include "filters/mbfilter_singlebyte.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\mbfl\\mbfl_convert.h （匹配3次）
	行 34: #include "mbfl_defs.h"
	行 35: #include "mbfl_encoding.h"
	行 36: #include "mbfl_memory_device.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\mbfl\\mbfl_encoding.c （匹配42次）
	行  31: #include "libmbfl/config.h"
	行  35: 	#include <strings.h>
	行  38: #include "mbfl_encoding.h"
	行  39: #include "mbfilter_pass.h"
	行  40: #include "mbfilter_8bit.h"
	行  42: #include "filters/mbfilter_euc_cn.h"
	行  43: #include "filters/mbfilter_hz.h"
	行  44: #include "filters/mbfilter_euc_tw.h"
	行  45: #include "filters/mbfilter_big5.h"
	行  46: #include "filters/mbfilter_uhc.h"
	行  47: #include "filters/mbfilter_euc_kr.h"
	行  48: #include "filters/mbfilter_iso2022_kr.h"
	行  49: #include "filters/mbfilter_sjis.h"
	行  50: #include "filters/mbfilter_sjis_mobile.h"
	行  51: #include "filters/mbfilter_sjis_mac.h"
	行  52: #include "filters/mbfilter_sjis_2004.h"
	行  53: #include "filters/mbfilter_cp51932.h"
	行  54: #include "filters/mbfilter_jis.h"
	行  55: #include "filters/mbfilter_iso2022_jp_ms.h"
	行  56: #include "filters/mbfilter_iso2022jp_2004.h"
	行  57: #include "filters/mbfilter_iso2022jp_mobile.h"
	行  58: #include "filters/mbfilter_euc_jp.h"
	行  59: #include "filters/mbfilter_euc_jp_win.h"
	行  60: #include "filters/mbfilter_euc_jp_2004.h"
	行  61: #include "filters/mbfilter_gb18030.h"
	行  62: #include "filters/mbfilter_cp932.h"
	行  63: #include "filters/mbfilter_cp936.h"
	行  64: #include "filters/mbfilter_cp5022x.h"
	行  65: #include "filters/mbfilter_base64.h"
	行  66: #include "filters/mbfilter_qprint.h"
	行  67: #include "filters/mbfilter_uuencode.h"
	行  68: #include "filters/mbfilter_7bit.h"
	行  69: #include "filters/mbfilter_utf7.h"
	行  70: #include "filters/mbfilter_utf7imap.h"
	行  71: #include "filters/mbfilter_utf8.h"
	行  72: #include "filters/mbfilter_utf8_mobile.h"
	行  73: #include "filters/mbfilter_utf16.h"
	行  74: #include "filters/mbfilter_utf32.h"
	行  75: #include "filters/mbfilter_ucs4.h"
	行  76: #include "filters/mbfilter_ucs2.h"
	行  77: #include "filters/mbfilter_htmlent.h"
	行  78: #include "filters/mbfilter_singlebyte.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\mbfl\\mbfl_encoding.h （匹配2次）
	行  34: #include "mbfl_defs.h"
	行  35: #include "zend.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\mbfl\\mbfl_filter_output.c （匹配2次）
	行 31: #include "mbfl_convert.h"
	行 32: #include "mbfl_filter_output.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\mbfl\\mbfl_language.c （匹配17次）
	行  31: #include "libmbfl/config.h"
	行  33: #include <stddef.h>
	行  34: #include <string.h>
	行  37: #include <strings.h>
	行  40: #include "mbfl_encoding.h"
	行  41: #include "mbfl_language.h"
	行  43: #include "nls/nls_ja.h"
	行  44: #include "nls/nls_kr.h"
	行  45: #include "nls/nls_zh.h"
	行  46: #include "nls/nls_uni.h"
	行  47: #include "nls/nls_de.h"
	行  48: #include "nls/nls_ru.h"
	行  49: #include "nls/nls_ua.h"
	行  50: #include "nls/nls_en.h"
	行  51: #include "nls/nls_hy.h"
	行  52: #include "nls/nls_tr.h"
	行  53: #include "nls/nls_neutral.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\mbfl\\mbfl_language.h （匹配2次）
	行 34: #include "mbfl_defs.h"
	行 35: #include "mbfl_encoding.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\mbfl\\mbfl_memory_device.c （匹配4次）
	行  31: #include <stddef.h>
	行  32: #include <string.h>
	行  34: #include "zend.h"
	行  35: #include "mbfl_memory_device.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\mbfl\\mbfl_memory_device.h （匹配2次）
	行 34: #include "mbfl_defs.h"
	行 35: #include "mbfl_string.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\mbfl\\mbfl_string.c （匹配2次）
	行 31: #include "mbfl_string.h"
	行 32: #include "mbfilter_pass.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\mbfl\\mbfl_string.h （匹配4次）
	行 34: #include <stddef.h>
	行 35: #include "mbfl_defs.h"
	行 36: #include "mbfl_encoding.h"
	行 37: #include "mbfl_language.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\nls\\nls_de.c （匹配1次）
	行  1: #include "nls_de.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\nls\\nls_de.h （匹配1次）
	行 4: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\nls\\nls_en.c （匹配2次）
	行  1: #include <stddef.h>
	行  2: #include "nls_en.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\nls\\nls_en.h （匹配1次）
	行 4: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\nls\\nls_hy.c （匹配2次）
	行  1: #include <stddef.h>
	行  2: #include "nls_hy.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\nls\\nls_hy.h （匹配2次）
	行  4: #include "mbfilter.h"
	行  5: #include "nls_hy.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\nls\\nls_ja.c （匹配2次）
	行  1: #include <stddef.h>
	行  2: #include "nls_ja.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\nls\\nls_ja.h （匹配1次）
	行 4: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\nls\\nls_kr.c （匹配2次）
	行  1: #include <stddef.h>
	行  2: #include "nls_kr.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\nls\\nls_kr.h （匹配1次）
	行 4: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\nls\\nls_neutral.c （匹配2次）
	行  1: #include <stddef.h>
	行  2: #include "nls_neutral.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\nls\\nls_neutral.h （匹配1次）
	行 4: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\nls\\nls_ru.c （匹配2次）
	行  1: #include <stddef.h>
	行  2: #include "nls_ru.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\nls\\nls_ru.h （匹配2次）
	行  4: #include "mbfilter.h"
	行  5: #include "nls_ru.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\nls\\nls_tr.c （匹配2次）
	行  1: #include <stddef.h>
	行  2: #include "nls_tr.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\nls\\nls_tr.h （匹配1次）
	行 4: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\nls\\nls_ua.c （匹配2次）
	行  1: #include <stddef.h>
	行  2: #include "nls_ua.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\nls\\nls_ua.h （匹配2次）
	行  4: #include "mbfilter.h"
	行  5: #include "nls_ua.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\nls\\nls_uni.c （匹配1次）
	行  1: #include "nls_uni.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\nls\\nls_uni.h （匹配1次）
	行 4: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\nls\\nls_zh.c （匹配2次）
	行  1: #include <stddef.h>
	行  2: #include "nls_zh.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\libmbfl\\nls\\nls_zh.h （匹配1次）
	行  4: #include "mbfilter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\mbstring.c （匹配34次）
	行   21: #include "libmbfl/config.h"
	行   22: #include "php.h"
	行   23: #include "php_ini.h"
	行   24: #include "php_variables.h"
	行   25: #include "mbstring.h"
	行   26: #include "ext/standard/php_string.h"
	行   27: #include "ext/standard/php_mail.h"
	行   28: #include "ext/standard/exec.h"
	行   29: #include "ext/standard/url.h"
	行   30: #include "main/php_output.h"
	行   31: #include "ext/standard/info.h"
	行   32: #include "ext/pcre/php_pcre.h"
	行   34: #include "libmbfl/mbfl/mbfilter_8bit.h"
	行   35: #include "libmbfl/mbfl/mbfilter_pass.h"
	行   36: #include "libmbfl/mbfl/mbfilter_wchar.h"
	行   37: #include "libmbfl/mbfl/eaw_table.h"
	行   38: #include "libmbfl/filters/mbfilter_base64.h"
	行   39: #include "libmbfl/filters/mbfilter_qprint.h"
	行   40: #include "libmbfl/filters/mbfilter_htmlent.h"
	行   41: #include "libmbfl/filters/mbfilter_uuencode.h"
	行   42: #include "libmbfl/filters/mbfilter_ucs4.h"
	行   43: #include "libmbfl/filters/mbfilter_utf8.h"
	行   44: #include "libmbfl/filters/mbfilter_singlebyte.h"
	行   45: #include "libmbfl/filters/translit_kana_jisx0201_jisx0208.h"
	行   47: #include "php_variables.h"
	行   48: #include "php_globals.h"
	行   49: #include "rfc1867.h"
	行   50: #include "php_content_types.h"
	行   51: #include "SAPI.h"
	行   52: #include "php_unicode.h"
	行   53: #include "TSRM.h"
	行   55: #include "mb_gpc.h"
	行   61: #include "zend_multibyte.h"
	行   62: #include "mbstring_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\mbstring.h （匹配3次）
	行  22: #include "php_version.h"
	行  42: #include "libmbfl/mbfl/mbfilter.h"
	行  43: #include "SAPI.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\mb_gpc.c （匹配16次）
	行  19: #include "php.h"
	行  20: #include "php_ini.h"
	行  21: #include "php_variables.h"
	行  22: #include "libmbfl/mbfl/mbfilter_pass.h"
	行  23: #include "mbstring.h"
	行  24: #include "ext/standard/php_string.h"
	行  25: #include "ext/standard/php_mail.h"
	行  26: #include "ext/standard/url.h"
	行  27: #include "main/php_output.h"
	行  28: #include "ext/standard/info.h"
	行  30: #include "php_globals.h"
	行  31: #include "rfc1867.h"
	行  32: #include "php_content_types.h"
	行  33: #include "SAPI.h"
	行  34: #include "TSRM.h"
	行  36: #include "mb_gpc.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\mb_gpc.h （匹配1次）
	行 19: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\php_mbregex.c （匹配10次）
	行   17: #include "libmbfl/config.h"
	行   19: #include "php.h"
	行   20: #include "php_ini.h"
	行   24: #include "zend_smart_str.h"
	行   25: #include "ext/standard/info.h"
	行   26: #include "php_mbregex.h"
	行   27: #include "mbstring.h"
	行   28: #include "libmbfl/filters/mbfilter_utf8.h"
	行   30: #include "php_onig_compat.h" /* must come prior to the oniguruma header */
	行   31: #include <oniguruma.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\php_mbregex.h （匹配2次）
	行 22: #include "php.h"
	行 23: #include "zend.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mbstring\\php_unicode.c （匹配5次）
	行  31: #include "php.h"
	行  34: #include "mbstring.h"
	行  35: #include "php_unicode.h"
	行  36: #include "unicode_data.h"
	行  37: #include "libmbfl/mbfl/mbfilter_wchar.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqli\\mysqli.c （匹配16次）
	行  20: #include "config.h"
	行  23: #include <signal.h>
	行  25: #include "php.h"
	行  26: #include "php_ini.h"
	行  27: #include "ext/standard/info.h"
	行  28: #include "ext/standard/php_string.h"
	行  29: #include "php_mysqli.h"
	行  30: #include "php_mysqli_structs.h"
	行  31: #include "mysqli_priv.h"
	行  32: #include "zend_attributes.h"
	行  33: #include "zend_exceptions.h"
	行  34: #include "ext/spl/spl_exceptions.h"
	行  35: #include "zend_interfaces.h"
	行  36: #include "zend_attributes.h"
	行  37: #include "mysqli_arginfo.h"
	行 406: #include "ext/mysqlnd/mysqlnd_reverse_api.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqli\\mysqli_api.c （匹配10次）
	行   20: #include "config.h"
	行   23: #include <signal.h>
	行   25: #include "php.h"
	行   26: #include "php_ini.h"
	行   27: #include "php_globals.h"
	行   28: #include "ext/standard/info.h"
	行   29: #include "zend_smart_str.h"
	行   30: #include "php_mysqli_structs.h"
	行   31: #include "mysqli_priv.h"
	行   32: #include "ext/mysqlnd/mysql_float_to_double.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqli\\mysqli_driver.c （匹配7次）
	行 18: #include "config.h"
	行 21: #include <signal.h>
	行 23: #include "php.h"
	行 24: #include "php_ini.h"
	行 25: #include "ext/standard/info.h"
	行 26: #include "php_mysqli_structs.h"
	行 27: #include "zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqli\\mysqli_exception.c （匹配8次）
	行 18: #include "config.h"
	行 21: #include <signal.h>
	行 23: #include "php.h"
	行 24: #include "php_ini.h"
	行 25: #include "ext/standard/info.h"
	行 26: #include "php_mysqli_structs.h"
	行 27: #include "mysqli_priv.h"
	行 28: #include "zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqli\\mysqli_mysqlnd.h （匹配2次）
	行 23: #include "ext/mysqlnd/mysqlnd_libmysql_compat.h"
	行 24: #include "ext/mysqlnd/mysqlnd_portability.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqli\\mysqli_nonapi.c （匹配9次）
	行   20: #include "config.h"
	行   23: #include <signal.h>
	行   25: #include "php.h"
	行   26: #include "php_ini.h"
	行   27: #include "ext/standard/info.h"
	行   28: #include "zend_smart_str.h"
	行   29: #include "php_mysqli_structs.h"
	行   30: #include "mysqli_priv.h"
	行  638: #include "php_network.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqli\\mysqli_prop.c （匹配7次）
	行  19: #include "config.h"
	行  22: #include <signal.h>
	行  24: #include "php.h"
	行  25: #include "php_ini.h"
	行  26: #include "ext/standard/info.h"
	行  27: #include "php_mysqli_structs.h"
	行  28: #include "mysqli_priv.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqli\\mysqli_report.c （匹配5次）
	行 18: #include "config.h"
	行 21: #include "php.h"
	行 22: #include "php_ini.h"
	行 23: #include "ext/standard/info.h"
	行 24: #include "php_mysqli_structs.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqli\\mysqli_result_iterator.c （匹配7次）
	行  20: #include "config.h"
	行  23: #include <signal.h>
	行  25: #include "php.h"
	行  26: #include "php_ini.h"
	行  27: #include "php_mysqli_structs.h"
	行  28: #include "mysqli_priv.h"
	行  29: #include "zend_interfaces.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqli\\mysqli_warning.c （匹配7次）
	行  18: #include "config.h"
	行  21: #include <signal.h>
	行  23: #include "php.h"
	行  24: #include "php_ini.h"
	行  25: #include "ext/standard/info.h"
	行  26: #include "php_mysqli_structs.h"
	行  27: #include "mysqli_priv.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqli\\php_mysqli.h （匹配1次）
	行 25: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqli\\php_mysqli_structs.h （匹配4次）
	行  29: #include "ext/mysqlnd/mysqlnd.h"
	行  30: #include "mysqli_mysqlnd.h"
	行 129: #include <inttypes.h>
	行 134: #include "TSRM.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\config-win.h （匹配7次）
	行   9: #include <sys/locking.h>
	行  10: #include <windows.h>
	行  11: #include <math.h>			/* Because of rint() */
	行  12: #include <fcntl.h>
	行  13: #include <io.h>
	行  14: #include <malloc.h>
	行  16: #include <stdint.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd.h （匹配4次）
	行  49: #include "TSRM.h"
	行  52: #include "mysqlnd_portability.h"
	行  53: #include "mysqlnd_enum_n_def.h"
	行  54: #include "mysqlnd_structs.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_alloc.c （匹配7次）
	行  18: #include "php.h"
	行  19: #include "mysqlnd.h"
	行  20: #include "mysqlnd_priv.h"
	行  21: #include "mysqlnd_debug.h"
	行  22: #include "mysqlnd_wireprotocol.h"
	行  23: #include "mysqlnd_statistics.h"
	行 327: #include "zend_smart_str.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_auth.c （匹配17次）
	行   18: #include "php.h"
	行   19: #include "mysqlnd.h"
	行   20: #include "mysqlnd_structs.h"
	行   21: #include "mysqlnd_auth.h"
	行   22: #include "mysqlnd_wireprotocol.h"
	行   23: #include "mysqlnd_connection.h"
	行   24: #include "mysqlnd_priv.h"
	行   25: #include "mysqlnd_charset.h"
	行   26: #include "mysqlnd_debug.h"
	行  514: #include "ext/standard/sha1.h"
	行  683: #include <openssl/rsa.h>
	行  684: #include <openssl/pem.h>
	行  685: #include <openssl/err.h>
	行  739: #include <wincrypt.h>
	行  740: #include <bcrypt.h>
	行  967: #include "ext/hash/php_hash.h"
	行  968: #include "ext/hash/php_hash_sha.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_block_alloc.c （匹配5次）
	行 19: #include "php.h"
	行 20: #include "mysqlnd.h"
	行 21: #include "mysqlnd_block_alloc.h"
	行 22: #include "mysqlnd_debug.h"
	行 23: #include "mysqlnd_priv.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_charset.c （匹配5次）
	行  19: #include "php.h"
	行  20: #include "mysqlnd.h"
	行  21: #include "mysqlnd_priv.h"
	行  22: #include "mysqlnd_debug.h"
	行  23: #include "mysqlnd_charset.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_commands.c （匹配7次）
	行  18: #include "php.h"
	行  19: #include "mysqlnd.h"
	行  20: #include "mysqlnd_connection.h"
	行  21: #include "mysqlnd_priv.h"
	行  22: #include "mysqlnd_auth.h"
	行  23: #include "mysqlnd_wireprotocol.h"
	行  24: #include "mysqlnd_debug.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_connection.c （匹配16次）
	行   18: #include "php.h"
	行   19: #include "mysqlnd.h"
	行   20: #include "mysqlnd_connection.h"
	行   21: #include "mysqlnd_vio.h"
	行   22: #include "mysqlnd_protocol_frame_codec.h"
	行   23: #include "mysqlnd_auth.h"
	行   24: #include "mysqlnd_wireprotocol.h"
	行   25: #include "mysqlnd_priv.h"
	行   26: #include "mysqlnd_result.h"
	行   27: #include "mysqlnd_statistics.h"
	行   28: #include "mysqlnd_charset.h"
	行   29: #include "mysqlnd_debug.h"
	行   30: #include "mysqlnd_ext_plugin.h"
	行   31: #include "zend_smart_str.h"
	行 2096: #include "php_network.h"
	行 2211: #include "win32/select.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_debug.c （匹配4次）
	行  18: #include "php.h"
	行  19: #include "mysqlnd.h"
	行  20: #include "mysqlnd_priv.h"
	行  21: #include "mysqlnd_debug.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_debug.h （匹配4次）
	行  21: #include "mysqlnd_alloc.h"
	行  22: #include "zend_stack.h"
	行  84: #include "win32/time.h"
	行  86: #include <sys/time.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_driver.c （匹配13次）
	行  18: #include "php.h"
	行  19: #include "mysqlnd.h"
	行  20: #include "mysqlnd_vio.h"
	行  21: #include "mysqlnd_protocol_frame_codec.h"
	行  22: #include "mysqlnd_wireprotocol.h"
	行  23: #include "mysqlnd_connection.h"
	行  24: #include "mysqlnd_ps.h"
	行  25: #include "mysqlnd_plugin.h"
	行  26: #include "mysqlnd_priv.h"
	行  27: #include "mysqlnd_statistics.h"
	行  28: #include "mysqlnd_debug.h"
	行  29: #include "mysqlnd_reverse_api.h"
	行  30: #include "mysqlnd_ext_plugin.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_ext_plugin.c （匹配7次）
	行  19: #include "php.h"
	行  20: #include "mysqlnd.h"
	行  21: #include "mysqlnd_priv.h"
	行  22: #include "mysqlnd_result.h"
	行  23: #include "mysqlnd_debug.h"
	行  24: #include "mysqlnd_commands.h"
	行  25: #include "mysqlnd_ext_plugin.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_loaddata.c （匹配5次）
	行  18: #include "php.h"
	行  19: #include "mysqlnd.h"
	行  20: #include "mysqlnd_wireprotocol.h"
	行  21: #include "mysqlnd_priv.h"
	行  22: #include "mysqlnd_debug.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_plugin.c （匹配5次）
	行  18: #include "php.h"
	行  19: #include "mysqlnd.h"
	行  20: #include "mysqlnd_priv.h"
	行  21: #include "mysqlnd_statistics.h"
	行  22: #include "mysqlnd_debug.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_portability.h （匹配1次）
	行  41: #include <stdint.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_protocol_frame_codec.c （匹配9次）
	行  18: #include "php.h"
	行  19: #include "mysqlnd.h"
	行  20: #include "mysqlnd_connection.h"
	行  21: #include "mysqlnd_priv.h"
	行  22: #include "mysqlnd_read_buffer.h"
	行  23: #include "mysqlnd_wireprotocol.h"
	行  24: #include "mysqlnd_statistics.h"
	行  25: #include "mysqlnd_debug.h"
	行  27: #include <zlib.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_ps.c （匹配12次）
	行   18: #include "php.h"
	行   19: #include "mysqlnd.h"
	行   20: #include "mysqlnd_wireprotocol.h"
	行   21: #include "mysqlnd_connection.h"
	行   22: #include "mysqlnd_priv.h"
	行   23: #include "mysqlnd_ps.h"
	行   24: #include "mysqlnd_result.h"
	行   25: #include "mysqlnd_result_meta.h"
	行   26: #include "mysqlnd_statistics.h"
	行   27: #include "mysqlnd_debug.h"
	行   28: #include "mysqlnd_block_alloc.h"
	行   29: #include "mysqlnd_ext_plugin.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_ps_codec.c （匹配9次）
	行  18: #include <math.h>
	行  19: #include "php.h"
	行  20: #include "mysqlnd.h"
	行  21: #include "mysqlnd_wireprotocol.h"
	行  22: #include "mysqlnd_connection.h"
	行  23: #include "mysqlnd_ps.h"
	行  24: #include "mysqlnd_priv.h"
	行  25: #include "mysqlnd_debug.h"
	行  26: #include "mysql_float_to_double.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_read_buffer.c （匹配4次）
	行 18: #include "php.h"
	行 19: #include "mysqlnd.h"
	行 20: #include "mysqlnd_debug.h"
	行 21: #include "mysqlnd_read_buffer.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_result.c （匹配11次）
	行   18: #include "php.h"
	行   19: #include "mysqlnd.h"
	行   20: #include "mysqlnd_wireprotocol.h"
	行   21: #include "mysqlnd_block_alloc.h"
	行   22: #include "mysqlnd_connection.h"
	行   23: #include "mysqlnd_priv.h"
	行   24: #include "mysqlnd_result.h"
	行   25: #include "mysqlnd_result_meta.h"
	行   26: #include "mysqlnd_statistics.h"
	行   27: #include "mysqlnd_debug.h"
	行   28: #include "mysqlnd_ext_plugin.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_result_meta.c （匹配9次）
	行  18: #include "php.h"
	行  19: #include "mysqlnd.h"
	行  20: #include "mysqlnd_connection.h"
	行  21: #include "mysqlnd_ps.h"
	行  22: #include "mysqlnd_priv.h"
	行  23: #include "mysqlnd_result.h"
	行  24: #include "mysqlnd_wireprotocol.h"
	行  25: #include "mysqlnd_debug.h"
	行  26: #include "ext/standard/basic_functions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_reverse_api.c （匹配5次）
	行 19: #include "php.h"
	行 20: #include "mysqlnd.h"
	行 21: #include "mysqlnd_priv.h"
	行 22: #include "mysqlnd_debug.h"
	行 23: #include "mysqlnd_reverse_api.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_statistics.c （匹配5次）
	行  19: #include "php.h"
	行  20: #include "mysqlnd.h"
	行  21: #include "mysqlnd_priv.h"
	行  22: #include "mysqlnd_statistics.h"
	行  23: #include "mysqlnd_debug.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_structs.h （匹配1次）
	行   21: #include "zend_smart_str_public.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_vio.c （匹配9次）
	行  18: #include "php.h"
	行  19: #include "mysqlnd.h"
	行  20: #include "mysqlnd_priv.h"
	行  21: #include "mysqlnd_statistics.h"
	行  22: #include "mysqlnd_debug.h"
	行  23: #include "mysqlnd_ext_plugin.h"
	行  24: #include "php_network.h"
	行  27: #include <netinet/tcp.h>
	行  29: #include <winsock.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysqlnd_wireprotocol.c （匹配8次）
	行   18: #include "php.h"
	行   19: #include "mysqlnd.h"
	行   20: #include "mysqlnd_connection.h"
	行   21: #include "mysqlnd_ps.h"
	行   22: #include "mysqlnd_priv.h"
	行   23: #include "mysqlnd_wireprotocol.h"
	行   24: #include "mysqlnd_statistics.h"
	行   25: #include "mysqlnd_debug.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\mysql_float_to_double.h （匹配3次）
	行 20: #include "main/php.h"
	行 21: #include <float.h>
	行 22: #include "main/snprintf.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\mysqlnd\\php_mysqlnd.c （匹配8次）
	行  18: #include "php.h"
	行  19: #include "mysqlnd.h"
	行  20: #include "mysqlnd_priv.h"
	行  21: #include "mysqlnd_debug.h"
	行  22: #include "mysqlnd_statistics.h"
	行  23: #include "mysqlnd_reverse_api.h"
	行  24: #include "ext/standard/info.h"
	行  25: #include "zend_smart_str.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\oci8\\oci8.c （匹配10次）
	行   28: #include "config.h"
	行   31: #include "php.h"
	行   32: #include "ext/standard/info.h"
	行   33: #include "php_ini.h"
	行   34: #include "zend_attributes.h"
	行   35: #include "zend_smart_str.h"
	行   56: #include "php_oci8.h"
	行   57: #include "php_oci8_int.h"
	行   58: #include "zend_hash.h"
	行  135: #include "oci8_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\oci8\\oci8_collection.c （匹配6次）
	行  27: #include "config.h"
	行  30: #include "php.h"
	行  31: #include "ext/standard/info.h"
	行  32: #include "php_ini.h"
	行  36: #include "php_oci8.h"
	行  37: #include "php_oci8_int.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\oci8\\oci8_failover.c （匹配6次）
	行  29: #include "config.h"
	行  32: #include "php.h"
	行  33: #include "ext/standard/info.h"
	行  34: #include "php_ini.h"
	行  38: #include "php_oci8.h"
	行  39: #include "php_oci8_int.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\oci8\\oci8_interface.c （匹配6次）
	行   27: #include "config.h"
	行   30: #include "php.h"
	行   31: #include "ext/standard/info.h"
	行   32: #include "php_ini.h"
	行   36: #include "php_oci8.h"
	行   37: #include "php_oci8_int.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\oci8\\oci8_lob.c （匹配7次）
	行  27: #include "config.h"
	行  30: #include "php.h"
	行  31: #include "ext/standard/info.h"
	行  32: #include "php_ini.h"
	行  36: #include "php_oci8.h"
	行  37: #include "php_oci8_int.h"
	行  40: #include <fcntl.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\oci8\\oci8_statement.c （匹配6次）
	行   27: #include "config.h"
	行   30: #include "php.h"
	行   31: #include "ext/standard/info.h"
	行   32: #include "php_ini.h"
	行   36: #include "php_oci8.h"
	行   37: #include "php_oci8_int.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\oci8\\php_oci8_int.h （匹配3次）
	行  44: #include "oci8_dtrace_gen.h"
	行  55: #include "ext/standard/php_string.h"
	行  56: #include <oci.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\odbc\\odbc_utils.c （匹配2次）
	行 17: #include "php.h"
	行 18: #include "php_odbc_utils.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\odbc\\php_odbc.c （匹配15次）
	行   22: #include "config.h"
	行   25: #include "php.h"
	行   26: #include "php_globals.h"
	行   27: #include "zend_attributes.h"
	行   29: #include "ext/standard/info.h"
	行   30: #include "ext/standard/php_string.h"
	行   31: #include "ext/standard/php_standard.h"
	行   33: #include "php_odbc.h"
	行   34: #include "php_odbc_includes.h"
	行   35: #include "php_globals.h"
	行   38: #include "php_odbc_utils.h"
	行   42: #include <fcntl.h>
	行   43: #include "ext/standard/head.h"
	行   44: #include "php_ini.h"
	行   50: #include "odbc_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\odbc\\php_odbc.h （匹配2次）
	行 25: #include "TSRM.h"
	行 31: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\odbc\\php_odbc_includes.h （匹配27次）
	行  54: #include <sql.h>
	行  55: #include <sqlext.h>
	行  61: #include <WINDOWS.H>
	行  62: #include <sql.h>
	行  63: #include <sqlext.h>
	行  72: #include <WINDOWS.H>
	行  73: #include <sql.h>
	行  74: #include <sqlext.h>
	行  89: #include <sql.h>
	行  90: #include <sqlext.h>
	行  91: #include <iodbcext.h>
	行 106: #include <sql.h>
	行 107: #include <sqlext.h>
	行 113: #include <sql.h>
	行 114: #include <sqlext.h>
	行 120: #include <iodbc.h>
	行 121: #include <isql.h>
	行 122: #include <isqlext.h>
	行 123: #include <udbcext.h>
	行 138: #include <odbc.h>
	行 145: #include <odbc.h>
	行 151: #include <sqlcli1.h>
	行 154: #include <LibraryManager.h>
	行 160: #include <WINDOWS.H>
	行 161: #include <sql.h>
	行 162: #include <sqlext.h>
	行 166: #include <winsock2.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\dynasm\\dasm_arm.h （匹配4次）
	行   7: #include <stddef.h>
	行   8: #include <stdarg.h>
	行   9: #include <string.h>
	行  10: #include <stdlib.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\dynasm\\dasm_arm64.h （匹配4次）
	行   7: #include <stddef.h>
	行   8: #include <stdarg.h>
	行   9: #include <string.h>
	行  10: #include <stdlib.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\dynasm\\dasm_mips.h （匹配4次）
	行   7: #include <stddef.h>
	行   8: #include <stdarg.h>
	行   9: #include <string.h>
	行  10: #include <stdlib.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\dynasm\\dasm_ppc.h （匹配4次）
	行   7: #include <stddef.h>
	行   8: #include <stdarg.h>
	行   9: #include <string.h>
	行  10: #include <stdlib.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\dynasm\\dasm_proto.h （匹配2次）
	行 10: #include <stddef.h>
	行 11: #include <stdarg.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\dynasm\\dasm_x86.h （匹配4次）
	行   7: #include <stddef.h>
	行   8: #include <stdarg.h>
	行   9: #include <string.h>
	行  10: #include <stdlib.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\dynasm\\minilua.c （匹配11次）
	行   31: #include <stddef.h>
	行   32: #include <stdarg.h>
	行   33: #include <limits.h>
	行   34: #include <math.h>
	行   35: #include <ctype.h>
	行   36: #include <stdio.h>
	行   37: #include <stdlib.h>
	行   38: #include <string.h>
	行   39: #include <setjmp.h>
	行   40: #include <errno.h>
	行   41: #include <time.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\libudis86\\decode.c （匹配4次）
	行   26: #include "udint.h"
	行   27: #include "types.h"
	行   28: #include "extern.h"
	行   29: #include "decode.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\libudis86\\decode.h （匹配3次）
	行  29: #include "types.h"
	行  30: #include "udint.h"
	行  31: #include "itab.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\libudis86\\extern.h （匹配1次）
	行  33: #include "types.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\libudis86\\itab.c （匹配1次）
	行    2: #include "decode.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\libudis86\\syn-att.c （匹配6次）
	行  26: #include "types.h"
	行  27: #include "extern.h"
	行  28: #include "decode.h"
	行  29: #include "itab.h"
	行  30: #include "syn.h"
	行  31: #include "udint.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\libudis86\\syn-intel.c （匹配6次）
	行  26: #include "types.h"
	行  27: #include "extern.h"
	行  28: #include "decode.h"
	行  29: #include "itab.h"
	行  30: #include "syn.h"
	行  31: #include "udint.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\libudis86\\syn.c （匹配4次）
	行  26: #include "types.h"
	行  27: #include "decode.h"
	行  28: #include "syn.h"
	行  29: #include "udint.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\libudis86\\syn.h （匹配1次）
	行 29: #include "types.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\libudis86\\types.h （匹配1次）
	行 131: #include "itab.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\libudis86\\udint.h （匹配1次）
	行  29: #include "types.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\libudis86\\udis86.c （匹配3次）
	行  27: #include "udint.h"
	行  28: #include "extern.h"
	行  29: #include "decode.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\vtune\\ittnotify_config.h （匹配9次）
	行 122: #include <stddef.h>
	行 124: #include <tchar.h>
	行 126: #include <stdint.h>
	行 128: #include <wchar.h>
	行 247: #include <windows.h>
	行 254: #include <dlfcn.h>
	行 256: #include <wchar.h>
	行 264: #include <pthread.h>
	行 398: #include "ittnotify_types.h" /* For __itt_group_id definition */
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\vtune\\jitprofiling.c （匹配4次）
	行  59: #include "ittnotify_config.h"
	行  62: #include <windows.h>
	行  65: #include <stdlib.h>
	行  67: #include "jitprofiling.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\vtune\\jitprofiling.h （匹配3次）
	行  79:  * #include <jitprofiling.h>
	行 120:  * #include <jitprofiling.h>
	行 165:  * #include <jitprofiling.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\zend_elf.c （匹配8次）
	行  20: #include <sys/types.h>
	行  21: #include <sys/stat.h>
	行  23: #include <sys/sysctl.h>
	行  25: #include <FindDirectory.h>
	行  27: #include <fcntl.h>
	行  28: #include <unistd.h>
	行  30: #include "zend_API.h"
	行  31: #include "zend_elf.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\zend_jit.c （匹配26次）
	行   19: #include "main/php.h"
	行   20: #include "main/SAPI.h"
	行   21: #include "php_version.h"
	行   22: #include <ZendAccelerator.h>
	行   23: #include "zend_shared_alloc.h"
	行   24: #include "Zend/zend_execute.h"
	行   25: #include "Zend/zend_vm.h"
	行   26: #include "Zend/zend_exceptions.h"
	行   27: #include "Zend/zend_constants.h"
	行   28: #include "Zend/zend_closures.h"
	行   29: #include "Zend/zend_ini.h"
	行   30: #include "Zend/zend_observer.h"
	行   31: #include "zend_smart_str.h"
	行   32: #include "jit/zend_jit.h"
	行   36: #include "Optimizer/zend_func_info.h"
	行   37: #include "Optimizer/zend_ssa.h"
	行   38: #include "Optimizer/zend_inference.h"
	行   39: #include "Optimizer/zend_call_graph.h"
	行   40: #include "Optimizer/zend_dump.h"
	行   48: #include "jit/zend_jit_internal.h"
	行   51: #include <pthread.h>
	行   94: #include "dynasm/dasm_proto.h"
	行  717: #include "jit/zend_jit_helpers.c"
	行  718: #include "jit/zend_jit_disasm.c"
	行  727: #include "Zend/zend_cpuinfo.h"
	行 4428: #include "jit/zend_jit_trace.c"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\zend_jit_disasm.c （匹配2次）
	行  51: #include "zend_sort.h"
	行  58: #include <dlfcn.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\zend_jit_gdb.c （匹配3次）
	行  26: #include "zend_jit_gdb.h"
	行  27: #include "zend_elf.h"
	行  28: #include "zend_gdb.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\zend_jit_gdb.h （匹配1次）
	行 23: #include "zend_compile.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\zend_jit_helpers.c （匹配1次）
	行   19: #include "Zend/zend_API.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\zend_jit_internal.h （匹配1次）
	行  24: #include "zend_bitset.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\zend_jit_oprofile.c （匹配1次）
	行 21: #include <opagent.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\zend_jit_perf_dump.c （匹配13次）
	行  21: #include <stdio.h>
	行  22: #include <unistd.h>
	行  23: #include <time.h>
	行  24: #include <sys/mman.h>
	行  25: #include <sys/types.h>
	行  26: #include <sys/stat.h>
	行  27: #include <fcntl.h>
	行  31: #include <sys/syscall.h>
	行  46: #include <FindDirectory.h>
	行  49: #include "zend_elf.h"
	行  50: #include "zend_mmap.h"
	行 279: #include <os/log.h>
	行 280: #include <os/signpost.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\zend_jit_vm_helpers.c （匹配11次）
	行   20: #include "Zend/zend_execute.h"
	行   21: #include "Zend/zend_exceptions.h"
	行   22: #include "Zend/zend_vm.h"
	行   23: #include "Zend/zend_closures.h"
	行   24: #include "Zend/zend_constants.h"
	行   25: #include "Zend/zend_API.h"
	行   27: #include <ZendAccelerator.h>
	行   28: #include "Optimizer/zend_func_info.h"
	行   29: #include "Optimizer/zend_call_graph.h"
	行   30: #include "zend_jit.h"
	行   37: #include "zend_jit_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\jit\\zend_jit_vtune.c （匹配2次）
	行 23: #include "jit/vtune/jitprofiling.h"
	行 24: #include "jit/vtune/jitprofiling.c"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\shared_alloc_mmap.c （匹配8次）
	行  22: #include "zend_shared_alloc.h"
	行  26: #include <sys/types.h>
	行  27: #include <sys/stat.h>
	行  28: #include <stdio.h>
	行  29: #include <stdlib.h>
	行  30: #include <sys/mman.h>
	行  33: #include <mach/vm_statistics.h>
	行  36: #include "zend_execute.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\shared_alloc_posix.c （匹配8次）
	行 22: #include "zend_shared_alloc.h"
	行 26: #include <sys/types.h>
	行 27: #include <sys/stat.h>
	行 28: #include <stdio.h>
	行 29: #include <fcntl.h>
	行 30: #include <sys/mman.h>
	行 31: #include <unistd.h>
	行 32: #include <stdlib.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\shared_alloc_shm.c （匹配11次）
	行  22: #include "zend_shared_alloc.h"
	行  29: #include <sys/types.h>
	行  30: #include <sys/shm.h>
	行  31: #include <sys/ipc.h>
	行  32: #include <signal.h>
	行  33: #include <stdio.h>
	行  34: #include <stdlib.h>
	行  35: #include <unistd.h>
	行  36: #include <errno.h>
	行  38: #include <sys/stat.h>
	行  39: #include <fcntl.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\shared_alloc_win32.c （匹配12次）
	行  22: #include "php.h"
	行  23: #include "ZendAccelerator.h"
	行  24: #include "zend_shared_alloc.h"
	行  25: #include "zend_accelerator_util_funcs.h"
	行  26: #include "zend_execute.h"
	行  27: #include "zend_system_id.h"
	行  28: #include "SAPI.h"
	行  29: #include "tsrm_win32.h"
	行  30: #include "win32/winutil.h"
	行  31: #include <winbase.h>
	行  32: #include <process.h>
	行  33: #include <LMCONS.H>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\ZendAccelerator.c （匹配39次）
	行   22: #include "main/php.h"
	行   23: #include "main/php_globals.h"
	行   24: #include "zend.h"
	行   25: #include "zend_extensions.h"
	行   26: #include "zend_compile.h"
	行   27: #include "ZendAccelerator.h"
	行   28: #include "zend_persist.h"
	行   29: #include "zend_shared_alloc.h"
	行   30: #include "zend_accelerator_module.h"
	行   31: #include "zend_accelerator_blacklist.h"
	行   32: #include "zend_list.h"
	行   33: #include "zend_execute.h"
	行   34: #include "zend_vm.h"
	行   35: #include "zend_inheritance.h"
	行   36: #include "zend_exceptions.h"
	行   37: #include "zend_mmap.h"
	行   38: #include "zend_observer.h"
	行   39: #include "main/php_main.h"
	行   40: #include "main/SAPI.h"
	行   41: #include "main/php_streams.h"
	行   42: #include "main/php_open_temporary_file.h"
	行   43: #include "zend_API.h"
	行   44: #include "zend_ini.h"
	行   45: #include "zend_virtual_cwd.h"
	行   46: #include "zend_accelerator_util_funcs.h"
	行   47: #include "zend_accelerator_hash.h"
	行   48: #include "zend_file_cache.h"
	行   49: #include "ext/pcre/php_pcre.h"
	行   50: #include "ext/standard/md5.h"
	行   51: #include "ext/hash/php_hash.h"
	行   58: #include  <netdb.h>
	行   64: #include <io.h>
	行   65: #include <lmcons.h>
	行   77: #include <fcntl.h>
	行   78: #include <signal.h>
	行   79: #include <time.h>
	行   89: #include <sys/stat.h>
	行   90: #include <errno.h>
	行   93: #include <immintrin.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\ZendAccelerator.h （匹配6次）
	行  42: #include "zend_config.h"
	行  51: #include "zend_extensions.h"
	行  52: #include "zend_compile.h"
	行  54: #include "Optimizer/zend_optimizer.h"
	行  55: #include "zend_accelerator_hash.h"
	行  56: #include "zend_accelerator_debug.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\zend_accelerator_blacklist.c （匹配7次）
	行  22: #include "main/php.h"
	行  23: #include "main/fopen_wrappers.h"
	行  24: #include "ZendAccelerator.h"
	行  25: #include "zend_accelerator_blacklist.h"
	行  35: #include "win32/glob.h"
	行  37: #include <glob.h>
	行  41: #include "ext/pcre/php_pcre.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\zend_accelerator_debug.c （匹配5次）
	行  22: #include <stdio.h>
	行  23: #include <stdlib.h>
	行  24: #include <stdarg.h>
	行  25: #include <time.h>
	行  29: #include "ZendAccelerator.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\zend_accelerator_hash.c （匹配4次）
	行  22: #include "ZendAccelerator.h"
	行  23: #include "zend_accelerator_hash.h"
	行  24: #include "zend_hash.h"
	行  25: #include "zend_shared_alloc.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\zend_accelerator_hash.h （匹配1次）
	行 25: #include "zend.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\zend_accelerator_module.c （匹配13次）
	行  22: #include <time.h>
	行  24: #include "php.h"
	行  25: #include "ZendAccelerator.h"
	行  26: #include "zend_API.h"
	行  27: #include "zend_shared_alloc.h"
	行  28: #include "zend_accelerator_blacklist.h"
	行  29: #include "php_ini.h"
	行  30: #include "SAPI.h"
	行  31: #include "zend_virtual_cwd.h"
	行  32: #include "ext/standard/info.h"
	行  33: #include "ext/standard/php_filestat.h"
	行  34: #include "opcache_arginfo.h"
	行  37: #include "jit/zend_jit.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\zend_accelerator_util_funcs.c （匹配7次）
	行  22: #include "zend_API.h"
	行  23: #include "zend_constants.h"
	行  24: #include "zend_inheritance.h"
	行  25: #include "zend_accelerator_util_funcs.h"
	行  26: #include "zend_persist.h"
	行  27: #include "zend_shared_alloc.h"
	行  28: #include "zend_observer.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\zend_accelerator_util_funcs.h （匹配2次）
	行 25: #include "zend.h"
	行 26: #include "ZendAccelerator.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\zend_file_cache.c （匹配19次）
	行   19: #include "zend.h"
	行   20: #include "zend_virtual_cwd.h"
	行   21: #include "zend_compile.h"
	行   22: #include "zend_vm.h"
	行   23: #include "zend_interfaces.h"
	行   24: #include "zend_attributes.h"
	行   25: #include "zend_system_id.h"
	行   27: #include "php.h"
	行   29: #include "ext/standard/md5.h"
	行   32: #include "ZendAccelerator.h"
	行   33: #include "zend_file_cache.h"
	行   34: #include "zend_shared_alloc.h"
	行   35: #include "zend_accelerator_util_funcs.h"
	行   36: #include "zend_accelerator_hash.h"
	行   39: #include "jit/zend_jit.h"
	行   42: #include <sys/types.h>
	行   43: #include <sys/stat.h>
	行   44: #include <fcntl.h>
	行   47: #include <unistd.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\zend_persist.c （匹配10次）
	行   22: #include "zend.h"
	行   23: #include "ZendAccelerator.h"
	行   24: #include "zend_persist.h"
	行   25: #include "zend_extensions.h"
	行   26: #include "zend_shared_alloc.h"
	行   27: #include "zend_vm.h"
	行   28: #include "zend_constants.h"
	行   29: #include "zend_operators.h"
	行   30: #include "zend_interfaces.h"
	行   31: #include "zend_attributes.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\zend_persist_calc.c （匹配7次）
	行  22: #include "zend.h"
	行  23: #include "ZendAccelerator.h"
	行  24: #include "zend_persist.h"
	行  25: #include "zend_extensions.h"
	行  26: #include "zend_shared_alloc.h"
	行  27: #include "zend_operators.h"
	行  28: #include "zend_attributes.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\zend_shared_alloc.c （匹配4次）
	行  22: #include <errno.h>
	行  23: #include "ZendAccelerator.h"
	行  24: #include "zend_shared_alloc.h"
	行  28: #include <fcntl.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\opcache\\zend_shared_alloc.h （匹配2次）
	行  25: #include "zend.h"
	行  26: #include "ZendAccelerator.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\openssl\\openssl.c （匹配31次）
	行   24: #include "config.h"
	行   27: #include "php.h"
	行   28: #include "php_ini.h"
	行   29: #include "php_openssl.h"
	行   30: #include "zend_attributes.h"
	行   31: #include "zend_exceptions.h"
	行   34: #include "ext/standard/file.h"
	行   35: #include "ext/standard/info.h"
	行   36: #include "ext/standard/php_fopen_wrappers.h"
	行   37: #include "ext/standard/md5.h"
	行   38: #include "ext/standard/base64.h"
	行   44: #include <openssl/evp.h>
	行   45: #include <openssl/bn.h>
	行   46: #include <openssl/rsa.h>
	行   47: #include <openssl/dsa.h>
	行   48: #include <openssl/dh.h>
	行   49: #include <openssl/x509.h>
	行   50: #include <openssl/x509v3.h>
	行   51: #include <openssl/crypto.h>
	行   52: #include <openssl/pem.h>
	行   53: #include <openssl/err.h>
	行   54: #include <openssl/conf.h>
	行   55: #include <openssl/rand.h>
	行   56: #include <openssl/ssl.h>
	行   57: #include <openssl/pkcs12.h>
	行   58: #include <openssl/cms.h>
	行   60: #include <openssl/core_names.h>
	行   61: #include <openssl/param_build.h>
	行   65: #include <openssl/engine.h>
	行   69: #include <time.h>
	行  142: #include "openssl_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\openssl\\php_openssl.h （匹配3次）
	行  25: #include "php_version.h"
	行  28: #include <openssl/opensslv.h>
	行  65: #include <openssl/err.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\openssl\\xp_ssl.c （匹配18次）
	行   21: #include "config.h"
	行   24: #include "php.h"
	行   25: #include "ext/standard/file.h"
	行   26: #include "ext/standard/url.h"
	行   27: #include "streams/php_streams_int.h"
	行   28: #include "zend_smart_str.h"
	行   29: #include "php_openssl.h"
	行   30: #include "php_network.h"
	行   31: #include <openssl/ssl.h>
	行   32: #include <openssl/rsa.h>
	行   33: #include <openssl/x509.h>
	行   34: #include <openssl/x509v3.h>
	行   35: #include <openssl/err.h>
	行   36: #include <openssl/bn.h>
	行   37: #include <openssl/dh.h>
	行   40: #include "win32/winutil.h"
	行   41: #include "win32/time.h"
	行   42: #include <Wincrypt.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcntl\\pcntl.c （匹配16次）
	行   27: #include "config.h"
	行   30: #include "php.h"
	行   31: #include "php_ini.h"
	行   32: #include "ext/standard/info.h"
	行   33: #include "php_pcntl.h"
	行   34: #include "php_signal.h"
	行   35: #include "php_ticks.h"
	行   36: #include "zend_fibers.h"
	行   39: #include <sys/wait.h>
	行   40: #include <sys/time.h>
	行   41: #include <sys/resource.h>
	行   44: #include <errno.h>
	行   46: #include <sched.h>
	行   50: #include <sys/fork.h>
	行   59: #include "pcntl_arginfo.h"
	行   61: #include "Zend/zend_max_execution_timer.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcntl\\php_pcntl.h （匹配1次）
	行 27: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcntl\\php_signal.c （匹配4次）
	行 17: #include "TSRM.h"
	行 18: #include "php_signal.h"
	行 19: #include "Zend/zend.h"
	行 20: #include "Zend/zend_signal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcntl\\php_signal.h （匹配1次）
	行 17: #include <signal.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\config.h （匹配1次）
	行   2: #include <php_compat.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2.h （匹配4次）
	行   6: #included by applications that call PCRE2 functions.
	行  90: #include <limits.h>
	行  91: #include <stdlib.h>
	行  92: #include <inttypes.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_auto_possess.c （匹配2次）
	行   46: #include "config.h"
	行   50: #include "pcre2_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_chartables.c （匹配3次）
	行  21: /* The following #include is present because without it gcc 4.x may remove
	行  29: #include "config.h"
	行  32: #include "pcre2_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_compile.c （匹配3次）
	行    43: #include "config.h"
	行    50: #include "pcre2_internal.h"
	行    60: #include "pcre2_printint.c"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_config.c （匹配2次）
	行  42: #include "config.h"
	行  51: #include "pcre2_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_context.c （匹配2次）
	行  43: #include "config.h"
	行  46: #include "pcre2_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_convert.c （匹配2次）
	行   43: #include "config.h"
	行   46: #include "pcre2_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_dfa_match.c （匹配2次）
	行   76: #include "config.h"
	行   83: #include "pcre2_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_error.c （匹配2次）
	行  43: #include "config.h"
	行  46: #include "pcre2_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_extuni.c （匹配2次）
	行  49: #include "config.h"
	行  53: #include "pcre2_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_find_bracket.c （匹配2次）
	行  50: #include "config.h"
	行  53: #include "pcre2_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_internal.h （匹配11次）
	行   56: #include <ctype.h>
	行   57: #include <limits.h>
	行   58: #include <stddef.h>
	行   59: #include <stdio.h>
	行   60: #include <stdlib.h>
	行   61: #include <string.h>
	行   76: #include <valgrind/memcheck.h>
	行  112: internals, can #include pcre2.h first to get an application's-eye view.
	行  143: #include "pcre2.h"
	行  144: #include "pcre2_ucp.h"
	行 1979: #include "pcre2_intmodedep.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_intmodedep.h （匹配2次）
	行  43: file is #included by pcre2_internal.h if PCRE2_CODE_UNIT_WIDTH is defined.
	行  45: #included multiple times for different code unit widths by pcre2test in order
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_jit_compile.c （匹配6次）
	行    43: #include "config.h"
	行    46: #include "pcre2_internal.h"
	行    79: #include "sljit/sljitLir.c"
	行  5864: #include "pcre2_jit_simd_inc.h"
	行 14506: #include "pcre2_jit_match.c"
	行 14507: #include "pcre2_jit_misc.c"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_jit_simd_inc.h （匹配7次）
	行  771: #include <arm_neon.h>
	行  836: #include "pcre2_jit_neon_inc.h"
	行  845: #include "pcre2_jit_neon_inc.h"
	行  854: #include "pcre2_jit_neon_inc.h"
	行 1016: #include "pcre2_jit_neon_inc.h"
	行 1027: #include "pcre2_jit_neon_inc.h"
	行 1038: #include "pcre2_jit_neon_inc.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_match.c （匹配3次）
	行   43: #include "config.h"
	行   53: #include <stdarg.h>
	行   63: #include "pcre2_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_match_data.c （匹配2次）
	行  43: #include "config.h"
	行  46: #include "pcre2_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_newline.c （匹配2次）
	行  52: #include "config.h"
	行  55: #include "pcre2_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_ord2utf.c （匹配2次）
	行  47: #include "config.h"
	行  50: #include "pcre2_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_pattern_info.c （匹配2次）
	行  43: #include "config.h"
	行  46: #include "pcre2_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_printint.c （匹配2次）
	行  44: local functions. This source file is #included in pcre2test.c at each supported
	行  52: pcre2_internal.h, which is #included by pcre2test before this file. */
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_script_run.c （匹配2次）
	行  44: #include "config.h"
	行  47: #include "pcre2_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_serialize.c （匹配2次）
	行  46: #include "config.h"
	行  50: #include "pcre2_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_string_utils.c （匹配2次）
	行  47: #include "config.h"
	行  50: #include "pcre2_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_study.c （匹配2次）
	行   46: #include "config.h"
	行   49: #include "pcre2_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_substitute.c （匹配2次）
	行   43: #include "config.h"
	行   46: #include "pcre2_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_substring.c （匹配2次）
	行  43: #include "config.h"
	行  46: #include "pcre2_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_tables.c （匹配4次）
	行  42: PCRE2 code modules. The tables are also #included by the pcre2test program,
	行  49: #include "config.h"
	行  51: #include "pcre2_internal.h"
	行 230: #include "pcre2_ucptables.c"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_ucd.c （匹配4次）
	行   49: As well as being part of the PCRE2 library, this file is #included by the
	行   57: #include "config.h"
	行   59: #include "pcre2_internal.h"
	行  145: /* When #included in pcre2test, we don't need the table of digit sets, nor the
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_valid_utf.c （匹配3次）
	行  43: strings. This file is also #included by the pcre2test program, which uses
	行  49: #include "config.h"
	行  51: #include "pcre2_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\pcre2_xclass.c （匹配2次）
	行  47: #include "config.h"
	行  51: #include "pcre2_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\sljit\\sljitConfigInternal.h （匹配5次）
	行  32: #include <stdio.h>
	行  37: #include <stdlib.h>
	行 223: #include <x86intrin.h>
	行 365: #include <libkern/OSCacheControl.h>
	行 391: #include <sys/cachectl.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\sljit\\sljitExecAllocator.c （匹配5次）
	行 104: #include <TargetConditionals.h>
	行 108: #include <sys/utsname.h>
	行 109: #include <stdlib.h>
	行 144: #include <AvailabilityMacros.h>
	行 145: #include <pthread.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\sljit\\sljitLir.c （匹配8次）
	行   27: #include "sljitLir.h"
	行   31: #include <windows.h>
	行   38: #include <stdlib.h>
	行   39: #include <string.h>
	行  264: #include "sljitUtils.c"
	行  271: #include "sljitProtExecAllocator.c"
	行  273: #include "sljitWXExecAllocator.c"
	行  275: #include "sljitExecAllocator.c"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\sljit\\sljitLir.h （匹配4次）
	行   74: #include "sljitConfigPre.h"
	行   77: #include "sljitConfig.h"
	行   83: #include "sljitConfigInternal.h"
	行   86: #include "sljitConfigPost.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\sljit\\sljitNativeMIPS_common.c （匹配2次）
	行  762: #include "sljitNativeMIPS_32.c"
	行  764: #include "sljitNativeMIPS_64.c"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\sljit\\sljitNativePPC_common.c （匹配3次）
	行   42: #include <sys/cache.h>
	行  697: #include "sljitNativePPC_32.c"
	行  699: #include "sljitNativePPC_64.c"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\sljit\\sljitNativeS390X.c （匹配1次）
	行   27: #include <sys/auxv.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\sljit\\sljitNativeSPARC_common.c （匹配2次）
	行  506: #include "sljitNativeSPARC_32.c"
	行  508: #include "sljitNativeSPARC_64.c"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\sljit\\sljitNativeX86_common.c （匹配4次）
	行  299: #include <cmnintrin.h>
	行  301: #include <intrin.h>
	行  892: #include "sljitNativeX86_32.c"
	行  894: #include "sljitNativeX86_64.c"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\sljit\\sljitProtExecAllocator.c （匹配4次）
	行  86: #include <sys/stat.h>
	行  87: #include <fcntl.h>
	行  88: #include <stdio.h>
	行  89: #include <string.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\sljit\\sljitUtils.c （匹配6次）
	行  39: #include <pthread.h>
	行  76: #include <sys/types.h>
	行  77: #include <sys/mman.h>
	行  87: #include <fcntl.h>
	行 109: #include <pthread.h>
	行 146: #include <unistd.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\pcre2lib\\sljit\\sljitWXExecAllocator.c （匹配6次）
	行  58: #include <sys/types.h>
	行  59: #include <sys/mman.h>
	行  67: #include <sys/param.h>
	行  72: #include <sys/sysctl.h>
	行  73: #include <unistd.h>
	行 109: #include <pthread.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\php_pcre.c （匹配10次）
	行   17: #include "php.h"
	行   18: #include "php_ini.h"
	行   19: #include "php_globals.h"
	行   20: #include "php_pcre.h"
	行   21: #include "ext/standard/info.h"
	行   22: #include "ext/standard/basic_functions.h"
	行   23: #include "zend_smart_str.h"
	行   24: #include "SAPI.h"
	行   26: #include "ext/standard/php_string.h"
	行   53: #include "php_pcre_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pcre\\php_pcre.h （匹配4次）
	行 21: #include "pcre2lib/pcre2.h"
	行 23: #include "pcre2.h"
	行 26: #include <locale.h>
	行 35: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo\\pdo.c （匹配11次）
	行  20: #include "config.h"
	行  23: #include <ctype.h>
	行  24: #include "php.h"
	行  25: #include "php_ini.h"
	行  26: #include "ext/standard/info.h"
	行  27: #include "php_pdo.h"
	行  28: #include "php_pdo_driver.h"
	行  29: #include "php_pdo_int.h"
	行  30: #include "zend_exceptions.h"
	行  31: #include "ext/spl/spl_exceptions.h"
	行  32: #include "pdo_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo\\pdo_dbh.c （匹配14次）
	行   22: #include "config.h"
	行   25: #include "php.h"
	行   26: #include "php_ini.h"
	行   27: #include "ext/standard/info.h"
	行   28: #include "php_pdo.h"
	行   29: #include "php_pdo_driver.h"
	行   30: #include "php_pdo_int.h"
	行   31: #include "zend_attributes.h"
	行   32: #include "zend_exceptions.h"
	行   33: #include "zend_object_handlers.h"
	行   34: #include "zend_hash.h"
	行   35: #include "pdo_dbh_arginfo.h"
	行   36: #include "zend_observer.h"
	行   37: #include "zend_extensions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo\\pdo_sqlstate.c （匹配6次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  22: #include "php_ini.h"
	行  23: #include "ext/standard/info.h"
	行  24: #include "php_pdo.h"
	行  25: #include "php_pdo_driver.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo\\pdo_stmt.c （匹配12次）
	行   22: #include "config.h"
	行   25: #include "php.h"
	行   26: #include "php_ini.h"
	行   27: #include "ext/standard/info.h"
	行   28: #include "ext/standard/php_var.h"
	行   29: #include "php_pdo.h"
	行   30: #include "php_pdo_driver.h"
	行   31: #include "php_pdo_int.h"
	行   32: #include "zend_exceptions.h"
	行   33: #include "zend_interfaces.h"
	行   34: #include "php_memory_streams.h"
	行   35: #include "pdo_stmt_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo\\php_pdo.h （匹配2次）
	行 20: #include "zend.h"
	行 25: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo\\php_pdo_driver.h （匹配1次）
	行  20: #include "php_pdo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo\\php_pdo_error.h （匹配1次）
	行 20: #include "php_pdo_driver.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo\\php_pdo_int.h （匹配1次）
	行 22: #include "php_pdo_error.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_dblib\\dblib_driver.c （匹配8次）
	行  22: #include "php.h"
	行  23: #include "php_ini.h"
	行  24: #include "ext/standard/info.h"
	行  25: #include "pdo/php_pdo.h"
	行  26: #include "pdo/php_pdo_driver.h"
	行  27: #include "php_pdo_dblib.h"
	行  28: #include "php_pdo_dblib_int.h"
	行  29: #include "zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_dblib\\dblib_stmt.c （匹配8次）
	行  22: #include "php.h"
	行  23: #include "php_ini.h"
	行  24: #include "ext/standard/info.h"
	行  25: #include "pdo/php_pdo.h"
	行  26: #include "pdo/php_pdo_driver.h"
	行  27: #include "php_pdo_dblib.h"
	行  28: #include "php_pdo_dblib_int.h"
	行  29: #include "zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_dblib\\pdo_dblib.c （匹配8次）
	行  22: #include "php.h"
	行  23: #include "php_ini.h"
	行  24: #include "ext/standard/info.h"
	行  25: #include "pdo/php_pdo.h"
	行  26: #include "pdo/php_pdo_driver.h"
	行  27: #include "php_pdo_dblib.h"
	行  28: #include "php_pdo_dblib_int.h"
	行  29: #include "zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_dblib\\php_pdo_dblib.h （匹配1次）
	行 29: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_firebird\\firebird_driver.c （匹配9次）
	行   18: #include "config.h"
	行   25: #include "php.h"
	行   26: #include "zend_exceptions.h"
	行   27: #include "php_ini.h"
	行   28: #include "ext/standard/info.h"
	行   29: #include "pdo/php_pdo.h"
	行   30: #include "pdo/php_pdo_driver.h"
	行   31: #include "php_pdo_firebird.h"
	行   32: #include "php_pdo_firebird_int.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_firebird\\firebird_statement.c （匹配9次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  22: #include "php_ini.h"
	行  23: #include "ext/standard/info.h"
	行  24: #include "pdo/php_pdo.h"
	行  25: #include "pdo/php_pdo_driver.h"
	行  26: #include "php_pdo_firebird.h"
	行  27: #include "php_pdo_firebird_int.h"
	行  29: #include <time.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_firebird\\pdo_firebird.c （匹配8次）
	行 18: #include "config.h"
	行 21: #include "php.h"
	行 22: #include "php_ini.h"
	行 23: #include "ext/standard/info.h"
	行 24: #include "pdo/php_pdo.h"
	行 25: #include "pdo/php_pdo_driver.h"
	行 26: #include "php_pdo_firebird.h"
	行 27: #include "php_pdo_firebird_int.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_firebird\\php_pdo_firebird.h （匹配2次）
	行 23: #include "php_version.h"
	行 27: #include "TSRM.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_firebird\\php_pdo_firebird_int.h （匹配1次）
	行  20: #include <ibase.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_mysql\\mysql_driver.c （匹配10次）
	行  20: #include "config.h"
	行  23: #include "php.h"
	行  24: #include "php_ini.h"
	行  25: #include "ext/standard/info.h"
	行  26: #include "pdo/php_pdo.h"
	行  27: #include "pdo/php_pdo_driver.h"
	行  28: #include "php_pdo_mysql.h"
	行  29: #include "php_pdo_mysql_int.h"
	行  31: #include <mysqld_error.h>
	行  33: #include "zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_mysql\\mysql_statement.c （匹配8次）
	行  20: #include "config.h"
	行  23: #include "php.h"
	行  24: #include "php_ini.h"
	行  25: #include "ext/standard/info.h"
	行  26: #include "pdo/php_pdo.h"
	行  27: #include "pdo/php_pdo_driver.h"
	行  28: #include "php_pdo_mysql.h"
	行  29: #include "php_pdo_mysql_int.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_mysql\\pdo_mysql.c （匹配9次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  23: #include "php_ini.h"
	行  24: #include "ext/standard/info.h"
	行  25: #include "pdo/php_pdo.h"
	行  26: #include "pdo/php_pdo_driver.h"
	行  27: #include "php_pdo_mysql.h"
	行  28: #include "php_pdo_mysql_int.h"
	行  60: #include "ext/mysqlnd/mysqlnd_reverse_api.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_mysql\\php_pdo_mysql.h （匹配2次）
	行 23: #include "php_version.h"
	行 33: #include "TSRM.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_mysql\\php_pdo_mysql_int.h （匹配1次）
	行  64: #include "ext/mysqlnd/mysqlnd_debug.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_oci\\oci_driver.c （匹配9次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  22: #include "php_ini.h"
	行  23: #include "ext/standard/info.h"
	行  24: #include "pdo/php_pdo.h"
	行  25: #include "pdo/php_pdo_driver.h"
	行  26: #include "php_pdo_oci.h"
	行  27: #include "php_pdo_oci_int.h"
	行  28: #include "Zend/zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_oci\\oci_statement.c （匹配9次）
	行   18: #include "config.h"
	行   21: #include "php.h"
	行   22: #include "php_ini.h"
	行   23: #include "ext/standard/info.h"
	行   24: #include "pdo/php_pdo.h"
	行   25: #include "pdo/php_pdo_driver.h"
	行   26: #include "php_pdo_oci.h"
	行   27: #include "php_pdo_oci_int.h"
	行   28: #include "Zend/zend_extensions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_oci\\pdo_oci.c （匹配9次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  22: #include "php_ini.h"
	行  23: #include "ext/standard/info.h"
	行  24: #include "pdo/php_pdo.h"
	行  25: #include "pdo/php_pdo_driver.h"
	行  26: #include "php_pdo_oci.h"
	行  27: #include "php_pdo_oci_int.h"
	行  29: #include <TSRM/TSRM.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_oci\\php_pdo_oci.h （匹配2次）
	行 23: #include "php_version.h"
	行 27: #include "TSRM.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_oci\\php_pdo_oci_int.h （匹配1次）
	行  17: #include <oci.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_odbc\\odbc_driver.c （匹配10次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  22: #include "php_ini.h"
	行  23: #include "ext/standard/info.h"
	行  24: #include "pdo/php_pdo.h"
	行  25: #include "pdo/php_pdo_driver.h"
	行  27: #include "php_odbc_utils.h"
	行  28: #include "php_pdo_odbc.h"
	行  29: #include "php_pdo_odbc_int.h"
	行  30: #include "zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_odbc\\odbc_stmt.c （匹配8次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  22: #include "php_ini.h"
	行  23: #include "ext/standard/info.h"
	行  24: #include "pdo/php_pdo.h"
	行  25: #include "pdo/php_pdo_driver.h"
	行  26: #include "php_pdo_odbc.h"
	行  27: #include "php_pdo_odbc_int.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_odbc\\pdo_odbc.c （匹配8次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  22: #include "php_ini.h"
	行  23: #include "ext/standard/info.h"
	行  24: #include "pdo/php_pdo.h"
	行  25: #include "pdo/php_pdo_driver.h"
	行  26: #include "php_pdo_odbc.h"
	行  27: #include "php_pdo_odbc_int.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_odbc\\php_pdo_odbc.h （匹配2次）
	行 23: #include "php_version.h"
	行 27: #include "TSRM.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_pgsql\\pdo_pgsql.c （匹配8次）
	行 18: #include "config.h"
	行 21: #include "php.h"
	行 22: #include "php_ini.h"
	行 23: #include "ext/standard/info.h"
	行 24: #include "pdo/php_pdo.h"
	行 25: #include "pdo/php_pdo_driver.h"
	行 26: #include "php_pdo_pgsql.h"
	行 27: #include "php_pdo_pgsql_int.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_pgsql\\pgsql_driver.c （匹配14次）
	行   20: #include "config.h"
	行   23: #include "php.h"
	行   24: #include "php_ini.h"
	行   25: #include "ext/standard/info.h"
	行   26: #include "ext/standard/php_string.h"
	行   27: #include "main/php_network.h"
	行   28: #include "pdo/php_pdo.h"
	行   29: #include "pdo/php_pdo_driver.h"
	行   30: #include "pdo/php_pdo_error.h"
	行   31: #include "ext/standard/file.h"
	行   32: #include "php_pdo_pgsql.h"
	行   33: #include "php_pdo_pgsql_int.h"
	行   34: #include "zend_exceptions.h"
	行   35: #include "pgsql_driver_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_pgsql\\pgsql_statement.c （匹配9次）
	行  20: #include "config.h"
	行  23: #include "php.h"
	行  24: #include "php_ini.h"
	行  25: #include "ext/standard/info.h"
	行  26: #include "pdo/php_pdo.h"
	行  27: #include "pdo/php_pdo_driver.h"
	行  28: #include "php_pdo_pgsql.h"
	行  29: #include "php_pdo_pgsql_int.h"
	行  31: #include <netinet/in.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_pgsql\\php_pdo_pgsql.h （匹配3次）
	行 20: #include <libpq-fe.h>
	行 25: #include "php_version.h"
	行 29: #include "TSRM.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_pgsql\\php_pdo_pgsql_int.h （匹配3次）
	行  22: #include <libpq-fe.h>
	行  23: #include <libpq/libpq-fs.h>
	行  24: #include <php.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_sqlite\\pdo_sqlite.c （匹配9次）
	行 18: #include "config.h"
	行 21: #include "php.h"
	行 22: #include "php_ini.h"
	行 23: #include "ext/standard/info.h"
	行 24: #include "pdo/php_pdo.h"
	行 25: #include "pdo/php_pdo_driver.h"
	行 26: #include "php_pdo_sqlite.h"
	行 27: #include "php_pdo_sqlite_int.h"
	行 28: #include "zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_sqlite\\php_pdo_sqlite.h （匹配2次）
	行 23: #include "php_version.h"
	行 27: #include "TSRM.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_sqlite\\php_pdo_sqlite_int.h （匹配1次）
	行 20: #include <sqlite3.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_sqlite\\sqlite_driver.c （匹配10次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  22: #include "php_ini.h"
	行  23: #include "ext/standard/info.h"
	行  24: #include "pdo/php_pdo.h"
	行  25: #include "pdo/php_pdo_driver.h"
	行  26: #include "php_pdo_sqlite.h"
	行  27: #include "php_pdo_sqlite_int.h"
	行  28: #include "zend_exceptions.h"
	行  29: #include "sqlite_driver_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pdo_sqlite\\sqlite_statement.c （匹配8次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  22: #include "php_ini.h"
	行  23: #include "ext/standard/info.h"
	行  24: #include "pdo/php_pdo.h"
	行  25: #include "pdo/php_pdo_driver.h"
	行  26: #include "php_pdo_sqlite.h"
	行  27: #include "php_pdo_sqlite_int.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pgsql\\pgsql.c （匹配11次）
	行   21: #include <stdlib.h>
	行   26: #include "config.h"
	行   31: #include "php.h"
	行   32: #include "php_ini.h"
	行   33: #include "ext/standard/php_standard.h"
	行   34: #include "zend_smart_str.h"
	行   35: #include "ext/pcre/php_pcre.h"
	行   39: #include "php_pgsql.h"
	行   40: #include "php_globals.h"
	行   41: #include "zend_exceptions.h"
	行   65: #include "pgsql_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pgsql\\php_pgsql.h （匹配3次）
	行  28: #include "php_version.h"
	行  33: #include <libpq-fe.h>
	行  35: #include <libpq/libpq-fs.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\phar\\dirstream.c （匹配2次）
	行  21: #include "phar_internal.h"
	行  22: #include "dirstream.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\phar\\func_interceptors.c （匹配1次）
	行   19: #include "phar_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\phar\\phar.c （匹配5次）
	行   21: #include "phar_internal.h"
	行   22: #include "SAPI.h"
	行   23: #include "func_interceptors.h"
	行   24: #include "ext/standard/php_var.h"
	行 2492: #include "stub.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\phar\\phar_internal.h （匹配39次）
	行  21: #include "config.h"
	行  24: #include <time.h>
	行  25: #include "php.h"
	行  26: #include "tar.h"
	行  27: #include "php_ini.h"
	行  28: #include "zend_constants.h"
	行  29: #include "zend_execute.h"
	行  30: #include "zend_exceptions.h"
	行  31: #include "zend_hash.h"
	行  32: #include "zend_interfaces.h"
	行  33: #include "zend_operators.h"
	行  34: #include "zend_sort.h"
	行  35: #include "zend_vm.h"
	行  36: #include "zend_smart_str.h"
	行  37: #include "main/php_streams.h"
	行  38: #include "main/streams/php_stream_plain_wrapper.h"
	行  39: #include "main/SAPI.h"
	行  40: #include "main/php_main.h"
	行  41: #include "main/php_open_temporary_file.h"
	行  42: #include "ext/standard/info.h"
	行  43: #include "ext/standard/basic_functions.h"
	行  44: #include "ext/standard/file.h"
	行  45: #include "ext/standard/php_string.h"
	行  46: #include "ext/standard/url.h"
	行  47: #include "ext/standard/crc32.h"
	行  48: #include "ext/standard/md5.h"
	行  49: #include "ext/standard/sha1.h"
	行  50: #include "ext/standard/php_var.h"
	行  51: #include "ext/standard/php_versioning.h"
	行  52: #include "Zend/zend_virtual_cwd.h"
	行  53: #include "ext/spl/spl_array.h"
	行  54: #include "ext/spl/spl_directory.h"
	行  55: #include "ext/spl/spl_engine.h"
	行  56: #include "ext/spl/spl_exceptions.h"
	行  57: #include "ext/spl/spl_iterators.h"
	行  58: #include "php_phar.h"
	行  59: #include "ext/hash/php_hash.h"
	行  60: #include "ext/hash/php_hash_sha.h"
	行 197: #include "pharzip.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\phar\\phar_object.c （匹配3次）
	行   20: #include "phar_internal.h"
	行   21: #include "func_interceptors.h"
	行   22: #include "phar_object_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\phar\\php_phar.h （匹配1次）
	行 25: #include "ext/standard/basic_functions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\phar\\stream.c （匹配3次）
	行  21: #include "phar_internal.h"
	行  22: #include "stream.h"
	行  23: #include "dirstream.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\phar\\tar.c （匹配1次）
	行   20: #include "phar_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\phar\\util.c （匹配12次）
	行   21: #include "phar_internal.h"
	行   22: #include "ext/hash/php_hash_sha.h"
	行   26: #include <openssl/evp.h>
	行   27: #include <openssl/x509.h>
	行   28: #include <openssl/x509v3.h>
	行   29: #include <openssl/crypto.h>
	行   30: #include <openssl/pem.h>
	行   31: #include <openssl/err.h>
	行   32: #include <openssl/conf.h>
	行   33: #include <openssl/rand.h>
	行   34: #include <openssl/ssl.h>
	行   35: #include <openssl/pkcs12.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\phar\\zip.c （匹配1次）
	行   19: #include "phar_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\posix\\php_posix.h （匹配2次）
	行 21: #include "config.h"
	行 32: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\posix\\posix.c （匹配17次）
	行   18: #include "config.h"
	行   21: #include "php.h"
	行   22: #include <unistd.h>
	行   23: #include "ext/standard/info.h"
	行   24: #include "ext/standard/php_string.h"
	行   25: #include "php_posix.h"
	行   30: #include <sys/time.h>
	行   33: #include <sys/resource.h>
	行   34: #include <sys/utsname.h>
	行   35: #include <sys/types.h>
	行   36: #include <sys/stat.h>
	行   37: #include <signal.h>
	行   38: #include <sys/times.h>
	行   39: #include <errno.h>
	行   40: #include <grp.h>
	行   41: #include <pwd.h>
	行   49: #include "posix_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pspell\\php_pspell.h （匹配1次）
	行 23: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\pspell\\pspell.c （匹配9次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  23: #include <stdlib.h>
	行  24: #include <ctype.h>
	行  25: #include <stdio.h>
	行  32: #include "php_pspell.h"
	行  33: #include <pspell.h>
	行  34: #include "ext/standard/info.h"
	行  42: #include "pspell_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\random\\engine_combinedlcg.c （匹配3次）
	行  22: #include "php.h"
	行  23: #include "php_random.h"
	行  25: #include "Zend/zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\random\\engine_mt19937.c （匹配3次）
	行  30: #include "php.h"
	行  31: #include "php_random.h"
	行  33: #include "Zend/zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\random\\engine_pcgoneseq128xslrr64.c （匹配3次）
	行  23: #include "php.h"
	行  24: #include "php_random.h"
	行  26: #include "Zend/zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\random\\engine_secure.c （匹配3次）
	行 22: #include "php.h"
	行 23: #include "php_random.h"
	行 25: #include "Zend/zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\random\\engine_user.c （匹配2次）
	行 21: #include "php.h"
	行 22: #include "php_random.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\random\\engine_xoshiro256starstar.c （匹配3次）
	行  24: #include "php.h"
	行  25: #include "php_random.h"
	行  27: #include "Zend/zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\random\\random.c （匹配8次）
	行  22: #include <stdlib.h>
	行  23: #include <sys/stat.h>
	行  24: #include <fcntl.h>
	行  25: #include <math.h>
	行  27: #include "php.h"
	行  29: #include "Zend/zend_exceptions.h"
	行  31: #include "php_random.h"
	行  65: #include "random_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\random\\randomizer.c （匹配5次）
	行  21: #include "php.h"
	行  22: #include "php_random.h"
	行  24: #include "ext/standard/php_array.h"
	行  25: #include "ext/standard/php_string.h"
	行  27: #include "Zend/zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\readline\\php_readline.h （匹配1次）
	行 31: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\readline\\readline.c （匹配8次）
	行  20: #include "config.h"
	行  23: #include "php.h"
	行  24: #include "php_readline.h"
	行  25: #include "readline_cli.h"
	行  26: #include "readline_arginfo.h"
	行  35: #include <editline/readline.h>
	行  37: #include <readline/readline.h>
	行  38: #include <readline/history.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\readline\\readline_cli.c （匹配27次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  28: #include "php_globals.h"
	行  29: #include "php_variables.h"
	行  30: #include "zend_hash.h"
	行  31: #include "zend_modules.h"
	行  33: #include "SAPI.h"
	行  34: #include <locale.h>
	行  35: #include "zend.h"
	行  36: #include "zend_extensions.h"
	行  37: #include "php_ini.h"
	行  38: #include "php_globals.h"
	行  39: #include "php_main.h"
	行  40: #include "fopen_wrappers.h"
	行  41: #include "ext/standard/php_standard.h"
	行  42: #include "zend_smart_str.h"
	行  45: #include <unixlib/local.h>
	行  49: #include <editline/readline.h>
	行  51: #include <readline/readline.h>
	行  52: #include <readline/history.h>
	行  55: #include "zend_compile.h"
	行  56: #include "zend_execute.h"
	行  57: #include "zend_highlight.h"
	行  58: #include "zend_exceptions.h"
	行  60: #include "sapi/cli/cli.h"
	行  61: #include "readline_cli.h"
	行  64: #include <dlfcn.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\readline\\readline_cli.h （匹配2次）
	行 18: #include "php.h"
	行 19: #include "zend_smart_str_public.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\reflection\\php_reflection.c （匹配24次）
	行   22: #include "config.h"
	行   25: #include "php.h"
	行   26: #include "php_ini.h"
	行   27: #include "php_reflection.h"
	行   28: #include "ext/standard/info.h"
	行   29: #include "ext/standard/sha1.h"
	行   30: #include "ext/random/php_random.h"
	行   32: #include "zend.h"
	行   33: #include "zend_API.h"
	行   34: #include "zend_ast.h"
	行   35: #include "zend_attributes.h"
	行   36: #include "zend_exceptions.h"
	行   37: #include "zend_operators.h"
	行   38: #include "zend_constants.h"
	行   39: #include "zend_ini.h"
	行   40: #include "zend_interfaces.h"
	行   41: #include "zend_closures.h"
	行   42: #include "zend_generators.h"
	行   43: #include "zend_extensions.h"
	行   44: #include "zend_builtin_functions.h"
	行   45: #include "zend_smart_str.h"
	行   46: #include "zend_enum.h"
	行   47: #include "zend_fibers.h"
	行   51: #include "php_reflection_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\reflection\\php_reflection.h （匹配1次）
	行 20: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\session\\mod_files.c （匹配14次）
	行  53: #include "php.h"
	行  55: #include <sys/stat.h>
	行  56: #include <sys/types.h>
	行  59: #include <sys/file.h>
	行  63: #include <dirent.h>
	行  67: #include "win32/readdir.h"
	行  69: #include <time.h>
	行  71: #include <fcntl.h>
	行  72: #include <errno.h>
	行  75: #include <unistd.h>
	行  78: #include "php_session.h"
	行  79: #include "mod_files.h"
	行  80: #include "ext/standard/flock_compat.h"
	行  81: #include "php_open_temporary_file.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\session\\mod_mm.c （匹配11次）
	行  17: #include "php.h"
	行  21: #include <unistd.h>
	行  22: #include <mm.h>
	行  23: #include <time.h>
	行  24: #include <sys/stat.h>
	行  25: #include <sys/types.h>
	行  26: #include <fcntl.h>
	行  27: #include <stdint.h>
	行  29: #include "php_session.h"
	行  30: #include "mod_mm.h"
	行  31: #include "SAPI.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\session\\mod_mm.h （匹配1次）
	行 22: #include "php_session.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\session\\mod_user.c （匹配3次）
	行  17: #include "php.h"
	行  18: #include "php_session.h"
	行  19: #include "mod_user.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\session\\mod_user_class.c （匹配2次）
	行  17: #include "php.h"
	行  18: #include "php_session.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\session\\php_session.h （匹配3次）
	行  20: #include "ext/standard/php_var.h"
	行  21: #include "ext/hash/php_hash.h"
	行  25: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\session\\session.c （匹配22次）
	行   19: #include "config.h"
	行   22: #include "php.h"
	行   31: #include <sys/stat.h>
	行   32: #include <fcntl.h>
	行   34: #include "php_ini.h"
	行   35: #include "SAPI.h"
	行   36: #include "rfc1867.h"
	行   37: #include "php_variables.h"
	行   38: #include "php_session.h"
	行   39: #include "session_arginfo.h"
	行   40: #include "ext/standard/php_var.h"
	行   41: #include "ext/date/php_date.h"
	行   42: #include "ext/standard/url_scanner_ex.h"
	行   43: #include "ext/standard/info.h"
	行   44: #include "zend_smart_str.h"
	行   45: #include "ext/standard/url.h"
	行   46: #include "ext/standard/basic_functions.h"
	行   47: #include "ext/standard/head.h"
	行   48: #include "ext/random/php_random.h"
	行   50: #include "mod_files.h"
	行   51: #include "mod_user.h"
	行   54: #include "mod_mm.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\shmop\\php_shmop.h （匹配1次）
	行 25: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\shmop\\shmop.c （匹配7次）
	行  21: #include "config.h"
	行  24: #include "php.h"
	行  25: #include "php_ini.h"
	行  26: #include "php_shmop.h"
	行  27: #include "shmop_arginfo.h"
	行  33: #include "tsrm_win32.h"
	行  39: #include "ext/standard/info.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\simplexml\\php_simplexml.h （匹配13次）
	行 23: #include "php_version.h"
	行 27: #include "TSRM.h"
	行 30: #include "ext/libxml/php_libxml.h"
	行 31: #include <libxml/parser.h>
	行 32: #include <libxml/parserInternals.h>
	行 33: #include <libxml/tree.h>
	行 34: #include <libxml/uri.h>
	行 35: #include <libxml/xmlerror.h>
	行 36: #include <libxml/xinclude.h>
	行 37: #include <libxml/xpath.h>
	行 38: #include <libxml/xpathInternals.h>
	行 39: #include <libxml/xpointer.h>
	行 40: #include <libxml/xmlschemas.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\simplexml\\php_simplexml_exports.h （匹配1次）
	行 22: #include "php_simplexml.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\simplexml\\simplexml.c （匹配11次）
	行   20: #include "config.h"
	行   23: #include "php.h"
	行   26: #include "php_ini.h"
	行   27: #include "ext/standard/info.h"
	行   28: #include "ext/standard/php_string.h"
	行   29: #include "php_simplexml.h"
	行   30: #include "php_simplexml_exports.h"
	行   31: #include "simplexml_arginfo.h"
	行   32: #include "zend_exceptions.h"
	行   33: #include "zend_interfaces.h"
	行   34: #include "ext/spl/spl_iterators.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\skeleton\\skeleton.c （匹配4次）
	行  7: #include "php.h"
	行  8: #include "ext/standard/info.h"
	行  9: #include "php_%EXTNAME%.h"
	行 10: #include "%EXTNAME%_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\snmp\\php_snmp.h （匹配3次）
	行  37: #include "TSRM.h"
	行  40: #include <net-snmp/net-snmp-config.h>
	行  41: #include <net-snmp/net-snmp-includes.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\snmp\\snmp.c （匹配22次）
	行   23: #include "config.h"
	行   26: #include "php.h"
	行   27: #include "main/php_network.h"
	行   28: #include "ext/standard/info.h"
	行   29: #include "php_snmp.h"
	行   31: #include "zend_exceptions.h"
	行   32: #include "zend_smart_string.h"
	行   33: #include "ext/spl/spl_exceptions.h"
	行   37: #include <sys/types.h>
	行   38: #include <errno.h>
	行   40: #include <winsock2.h>
	行   41: #include <process.h>
	行   42: #include "win32/time.h"
	行   44: #include <sys/socket.h>
	行   45: #include <netinet/in.h>
	行   46: #include <arpa/inet.h>
	行   47: #include <netdb.h>
	行   50: #include <unistd.h>
	行   52: #include <locale.h>
	行   62: #include <net-snmp/net-snmp-config.h>
	行   63: #include <net-snmp/net-snmp-includes.h>
	行   65: #include "snmp_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\soap\\php_encoding.c （匹配7次）
	行   19: #include <time.h>
	行   21: #include "php_soap.h"
	行   22: #include "ext/libxml/php_libxml.h"
	行   23: #include "ext/standard/base64.h"
	行   24: #include <libxml/parserInternals.h>
	行   25: #include "zend_strtod.h"
	行   26: #include "zend_interfaces.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\soap\\php_http.c （匹配4次）
	行   19: #include "php_soap.h"
	行   20: #include "ext/standard/base64.h"
	行   21: #include "ext/standard/md5.h"
	行   22: #include "ext/random/php_random.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\soap\\php_packet_soap.c （匹配1次）
	行  19: #include "php_soap.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\soap\\php_schema.c （匹配2次）
	行   19: #include "php_soap.h"
	行   20: #include "libxml/uri.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\soap\\php_sdl.c （匹配8次）
	行   19: #include "php_soap.h"
	行   20: #include "ext/libxml/php_libxml.h"
	行   21: #include "libxml/uri.h"
	行   23: #include "ext/standard/md5.h"
	行   24: #include "zend_virtual_cwd.h"
	行   26: #include <sys/types.h>
	行   27: #include <sys/stat.h>
	行   28: #include <fcntl.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\soap\\php_soap.h （匹配17次）
	行  22: #include "php.h"
	行  23: #include "php_globals.h"
	行  24: #include "ext/standard/info.h"
	行  25: #include "ext/standard/php_standard.h"
	行  27: #include "ext/session/php_session.h"
	行  29: #include "zend_smart_str.h"
	行  30: #include "php_ini.h"
	行  31: #include "SAPI.h"
	行  32: #include <libxml/parser.h>
	行  33: #include <libxml/xpath.h>
	行  64: #include "php_xml.h"
	行  65: #include "php_encoding.h"
	行  66: #include "php_sdl.h"
	行  67: #include "php_schema.h"
	行  68: #include "php_http.h"
	行  69: #include "php_packet_soap.h"
	行 181: #include "TSRM.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\soap\\php_xml.c （匹配4次）
	行  19: #include "php_soap.h"
	行  20: #include "ext/libxml/php_libxml.h"
	行  21: #include "libxml/parser.h"
	行  22: #include "libxml/parserInternals.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\soap\\soap.c （匹配6次）
	行   20: #include "config.h"
	行   22: #include "php_soap.h"
	行   24: #include "ext/session/php_session.h"
	行   26: #include "soap_arginfo.h"
	行   27: #include "zend_exceptions.h"
	行   28: #include "zend_interfaces.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\sockets\\conversions.c （匹配8次）
	行    5: #include "sockaddr_conv.h"
	行    6: #include "conversions.h"
	行    7: #include "sendrecvmsg.h" /* for ancillary registry */
	行   12: #include <Zend/zend_llist.h>
	行   13: #include <zend_smart_str.h>
	行   27: #include <limits.h>
	行   28: #include <stdarg.h>
	行   29: #include <stddef.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\sockets\\conversions.h （匹配2次）
	行  4: #include <php.h>
	行 23: #include "php_sockets.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\sockets\\multicast.c （匹配13次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  23: #include "php_network.h"
	行  27: #include <sys/socket.h>
	行  28: #include <sys/ioctl.h>
	行  29: #include <net/if.h>
	行  31: #include <sys/sockio.h>
	行  33: #include <netinet/in.h>
	行  34: #include <arpa/inet.h>
	行  37: #include "php_sockets.h"
	行  38: #include "multicast.h"
	行  39: #include "sockaddr_conv.h"
	行  40: #include "main/php_network.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\sockets\\php_sockets.h （匹配3次）
	行  29: #include <php.h>
	行  42: #include <Winsock2.h>
	行  45: #include <sys/socket.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\sockets\\sendrecvmsg.c （匹配9次）
	行  21: #include <php.h>
	行  22: #include "php_sockets.h"
	行  23: #include "sendrecvmsg.h"
	行  24: #include "conversions.h"
	行  25: #include <limits.h>
	行  26: #include <Zend/zend_llist.h>
	行  28: #include <TSRM/TSRM.h>
	行  36: #include "windows_common.h"
	行  37: #include <Mswsock.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\sockets\\sendrecvmsg.h （匹配2次）
	行  4: #include <php.h>
	行  5: #include "conversions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\sockets\\sockaddr_conv.c （匹配6次）
	行   1: #include <php.h>
	行   2: #include <php_network.h>
	行   3: #include "php_sockets.h"
	行   6: #include "windows_common.h"
	行   8: #include <netdb.h>
	行   9: #include <arpa/inet.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\sockets\\sockaddr_conv.h （匹配2次）
	行  4: #include <php_network.h>
	行  5: #include "php_sockets.h" /* php_socket */
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\sockets\\sockets.c （匹配11次）
	行   22: #include "config.h"
	行   25: #include "php.h"
	行   27: #include "php_network.h"
	行   28: #include "ext/standard/file.h"
	行   29: #include "ext/standard/info.h"
	行   30: #include "php_ini.h"
	行   70: #include <stddef.h>
	行   72: #include "sockaddr_conv.h"
	行   73: #include "multicast.h"
	行   74: #include "sendrecvmsg.h"
	行   75: #include "sockets_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\sockets\\windows_common.h （匹配2次）
	行  17: #include <Winsock2.h>
	行  19: #include <IPHlpApi.h> /* conflicting definition of CMSG_DATA */
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\sodium\\libsodium.c （匹配10次）
	行   21: #include "php.h"
	行   22: #include "php_ini.h"
	行   23: #include "ext/standard/info.h"
	行   24: #include "php_libsodium.h"
	行   25: #include "zend_attributes.h"
	行   26: #include "zend_exceptions.h"
	行   28: #include <sodium.h>
	行   29: #include <stdint.h>
	行   30: #include <string.h>
	行   68: #include "libsodium_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\sodium\\sodium_pwhash.c （匹配4次）
	行  21: #include "php.h"
	行  22: #include "php_libsodium.h"
	行  23: #include "ext/standard/php_password.h"
	行  25: #include <sodium.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\spl\\php_spl.c （匹配20次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  22: #include "php_ini.h"
	行  23: #include "php_main.h"
	行  24: #include "ext/standard/info.h"
	行  25: #include "php_spl.h"
	行  26: #include "php_spl_arginfo.h"
	行  27: #include "spl_functions.h"
	行  28: #include "spl_engine.h"
	行  29: #include "spl_array.h"
	行  30: #include "spl_directory.h"
	行  31: #include "spl_iterators.h"
	行  32: #include "spl_exceptions.h"
	行  33: #include "spl_observer.h"
	行  34: #include "spl_dllist.h"
	行  35: #include "spl_fixedarray.h"
	行  36: #include "spl_heap.h"
	行  37: #include "zend_exceptions.h"
	行  38: #include "zend_interfaces.h"
	行  39: #include "main/snprintf.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\spl\\php_spl.h （匹配2次）
	行 20: #include "php.h"
	行 21: #include <stdarg.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\spl\\spl_array.c （匹配13次）
	行   21: #include "php.h"
	行   22: #include "php_ini.h"
	行   23: #include "ext/standard/info.h"
	行   24: #include "ext/standard/php_var.h"
	行   25: #include "zend_smart_str.h"
	行   26: #include "zend_interfaces.h"
	行   27: #include "zend_exceptions.h"
	行   29: #include "php_spl.h"
	行   30: #include "spl_functions.h"
	行   31: #include "spl_iterators.h"
	行   32: #include "spl_array.h"
	行   33: #include "spl_array_arginfo.h"
	行   34: #include "spl_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\spl\\spl_array.h （匹配3次）
	行 20: #include "php.h"
	行 21: #include "php_spl.h"
	行 22: #include "spl_iterators.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\spl\\spl_directory.c （匹配16次）
	行   21: #include "php.h"
	行   22: #include "php_ini.h"
	行   23: #include "ext/standard/file.h"
	行   24: #include "ext/standard/php_filestat.h"
	行   25: #include "ext/standard/flock_compat.h"
	行   26: #include "ext/standard/scanf.h"
	行   27: #include "ext/standard/php_string.h"
	行   28: #include "zend_exceptions.h"
	行   29: #include "zend_interfaces.h"
	行   31: #include "php_spl.h"
	行   32: #include "spl_functions.h"
	行   33: #include "spl_engine.h"
	行   34: #include "spl_iterators.h"
	行   35: #include "spl_directory.h"
	行   36: #include "spl_directory_arginfo.h"
	行   37: #include "spl_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\spl\\spl_directory.h （匹配2次）
	行  20: #include "php.h"
	行  21: #include "php_spl.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\spl\\spl_dllist.c （匹配13次）
	行   21: #include "php.h"
	行   22: #include "zend_exceptions.h"
	行   23: #include "zend_hash.h"
	行   25: #include "php_spl.h"
	行   26: #include "ext/standard/info.h"
	行   27: #include "ext/standard/php_var.h"
	行   28: #include "zend_smart_str.h"
	行   29: #include "spl_functions.h"
	行   30: #include "spl_engine.h"
	行   31: #include "spl_iterators.h"
	行   32: #include "spl_dllist.h"
	行   33: #include "spl_dllist_arginfo.h"
	行   34: #include "spl_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\spl\\spl_dllist.h （匹配2次）
	行 20: #include "php.h"
	行 21: #include "php_spl.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\spl\\spl_engine.h （匹配3次）
	行 20: #include "php.h"
	行 21: #include "php_spl.h"
	行 22: #include "zend_interfaces.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\spl\\spl_exceptions.c （匹配10次）
	行 21: #include "php.h"
	行 22: #include "php_ini.h"
	行 23: #include "ext/standard/info.h"
	行 24: #include "zend_interfaces.h"
	行 25: #include "zend_exceptions.h"
	行 26: #include "spl_exceptions_arginfo.h"
	行 28: #include "php_spl.h"
	行 29: #include "spl_functions.h"
	行 30: #include "spl_engine.h"
	行 31: #include "spl_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\spl\\spl_exceptions.h （匹配2次）
	行 20: #include "php.h"
	行 21: #include "php_spl.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\spl\\spl_fixedarray.c （匹配13次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  23: #include "php_ini.h"
	行  24: #include "ext/standard/info.h"
	行  25: #include "zend_exceptions.h"
	行  27: #include "php_spl.h"
	行  28: #include "spl_fixedarray_arginfo.h"
	行  29: #include "spl_functions.h"
	行  30: #include "spl_engine.h"
	行  31: #include "spl_fixedarray.h"
	行  32: #include "spl_exceptions.h"
	行  33: #include "spl_iterators.h"
	行  34: #include "ext/json/php_json.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\spl\\spl_functions.c （匹配3次）
	行 18: 	#include "config.h"
	行 21: #include "php.h"
	行 22: #include "php_spl.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\spl\\spl_functions.h （匹配1次）
	行 20: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\spl\\spl_heap.c （匹配9次）
	行   21: #include "php.h"
	行   22: #include "zend_exceptions.h"
	行   24: #include "php_spl.h"
	行   25: #include "spl_functions.h"
	行   26: #include "spl_engine.h"
	行   27: #include "spl_iterators.h"
	行   28: #include "spl_heap.h"
	行   29: #include "spl_heap_arginfo.h"
	行   30: #include "spl_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\spl\\spl_heap.h （匹配2次）
	行 20: #include "php.h"
	行 21: #include "php_spl.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\spl\\spl_iterators.c （匹配15次）
	行   21: #include "php.h"
	行   22: #include "php_ini.h"
	行   23: #include "ext/standard/info.h"
	行   24: #include "zend_exceptions.h"
	行   25: #include "zend_interfaces.h"
	行   26: #include "ext/pcre/php_pcre.h"
	行   28: #include "php_spl.h"
	行   29: #include "spl_functions.h"
	行   30: #include "spl_engine.h"
	行   31: #include "spl_iterators.h"
	行   32: #include "spl_iterators_arginfo.h"
	行   33: #include "spl_directory.h"
	行   34: #include "spl_array.h"
	行   35: #include "spl_exceptions.h"
	行   36: #include "zend_smart_str.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\spl\\spl_iterators.h （匹配2次）
	行  20: #include "php.h"
	行  21: #include "php_spl.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\spl\\spl_observer.c （匹配16次）
	行   22: #include "php.h"
	行   23: #include "php_ini.h"
	行   24: #include "ext/standard/info.h"
	行   25: #include "ext/standard/php_array.h"
	行   26: #include "ext/standard/php_var.h"
	行   27: #include "zend_smart_str.h"
	行   28: #include "zend_interfaces.h"
	行   29: #include "zend_exceptions.h"
	行   31: #include "php_spl.h"
	行   32: #include "spl_functions.h"
	行   33: #include "spl_engine.h"
	行   34: #include "spl_observer.h"
	行   35: #include "spl_observer_arginfo.h"
	行   36: #include "spl_iterators.h"
	行   37: #include "spl_array.h"
	行   38: #include "spl_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\spl\\spl_observer.h （匹配2次）
	行 20: #include "php.h"
	行 21: #include "php_spl.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\sqlite3\\php_sqlite3_structs.h （匹配1次）
	行  20: #include <sqlite3.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\sqlite3\\sqlite3.c （匹配11次）
	行   18: #include "config.h"
	行   21: #include "php.h"
	行   22: #include "php_ini.h"
	行   23: #include "ext/standard/info.h"
	行   24: #include "php_sqlite3.h"
	行   25: #include "php_sqlite3_structs.h"
	行   26: #include "main/SAPI.h"
	行   28: #include <sqlite3.h>
	行   30: #include "zend_exceptions.h"
	行   31: #include "SAPI.h"
	行   32: #include "sqlite3_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\array.c （匹配21次）
	行   22: #include "php.h"
	行   23: #include "php_ini.h"
	行   24: #include <stdarg.h>
	行   25: #include <stdlib.h>
	行   26: #include <math.h>
	行   27: #include <time.h>
	行   28: #include <stdio.h>
	行   29: #include <string.h>
	行   31: #include "win32/unistd.h"
	行   33: #include "zend_globals.h"
	行   34: #include "zend_interfaces.h"
	行   35: #include "php_globals.h"
	行   36: #include "php_array.h"
	行   37: #include "basic_functions.h"
	行   38: #include "php_string.h"
	行   39: #include "php_math.h"
	行   40: #include "zend_smart_str.h"
	行   41: #include "zend_bitset.h"
	行   42: #include "zend_exceptions.h"
	行   43: #include "ext/spl/spl_array.h"
	行   44: #include "ext/random/php_random.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\assert.c （匹配4次）
	行  18: #include "php.h"
	行  19: #include "php_assert.h"
	行  20: #include "php_ini.h"
	行  21: #include "zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\base64.c （匹配4次）
	行  18: #include <string.h>
	行  20: #include "php.h"
	行  21: #include "base64.h"
	行  55: #include <arm_neon.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\basic_functions.c （匹配47次）
	行   18: #include "php.h"
	行   19: #include "php_streams.h"
	行   20: #include "php_main.h"
	行   21: #include "php_globals.h"
	行   22: #include "php_variables.h"
	行   23: #include "php_ini.h"
	行   24: #include "php_image.h"
	行   25: #include "php_standard.h"
	行   26: #include "php_math.h"
	行   27: #include "php_http.h"
	行   28: #include "php_incomplete_class.h"
	行   29: #include "php_getopt.h"
	行   30: #include "php_ext_syslog.h"
	行   31: #include "ext/standard/info.h"
	行   32: #include "ext/session/php_session.h"
	行   33: #include "zend_exceptions.h"
	行   34: #include "zend_attributes.h"
	行   35: #include "zend_ini.h"
	行   36: #include "zend_operators.h"
	行   37: #include "ext/standard/php_dns.h"
	行   38: #include "ext/standard/php_uuencode.h"
	行   39: #include "ext/standard/crc32_x86.h"
	行   42: #include "win32/php_win32_globals.h"
	行   43: #include "win32/time.h"
	行   44: #include "win32/ioutil.h"
	行   49: #include "zend.h"
	行   50: #include "zend_ini_scanner.h"
	行   51: #include "zend_language_scanner.h"
	行   52: #include <zend_language_parser.h>
	行   54: #include "zend_portability.h"
	行   56: #include <stdarg.h>
	行   57: #include <stdlib.h>
	行   58: #include <math.h>
	行   59: #include <time.h>
	行   60: #include <stdio.h>
	行   63: #include <sys/types.h>
	行   64: #include <sys/stat.h>
	行   70: #include "win32/inet.h"
	行   81: #include <string.h>
	行   82: #include <locale.h>
	行  100: #include "zend_globals.h"
	行  101: #include "php_globals.h"
	行  102: #include "SAPI.h"
	行  103: #include "php_ticks.h"
	行  111: #include "php_fopen_wrappers.h"
	行  112: #include "streamsfuncs.h"
	行  113: #include "basic_functions_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\basic_functions.h （匹配7次）
	行  21: #include <sys/stat.h>
	行  22: #include <wchar.h>
	行  24: #include "php_filestat.h"
	行  26: #include "zend_highlight.h"
	行  28: #include "url_scanner_ex.h"
	行  31: #include "ext/random/php_random.h"
	行  34: #include <intrin.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\browscap.c （匹配7次）
	行  17: #include "php.h"
	行  18: #include "php_browscap.h"
	行  19: #include "php_ini.h"
	行  20: #include "php_string.h"
	行  21: #include "ext/pcre/php_pcre.h"
	行  23: #include "zend_ini_scanner.h"
	行  24: #include "zend_globals.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\crc32.c （匹配4次）
	行  17: #include "php.h"
	行  18: #include "basic_functions.h"
	行  19: #include "crc32.h"
	行  20: #include "crc32_x86.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\crc32_x86.c （匹配1次）
	行  20: #include "crc32_x86.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\crc32_x86.h （匹配1次）
	行 20: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\credits.c （匹配5次）
	行  18: #include "php.h"
	行  19: #include "info.h"
	行  20: #include "SAPI.h"
	行  78: #include "credits_sapi.h"
	行  88: #include "credits_ext.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\crypt.c （匹配8次）
	行  20: #include <stdlib.h>
	行  22: #include "php.h"
	行  25: #include <unistd.h>
	行  38: #include <time.h>
	行  39: #include <string.h>
	行  42: #include <process.h>
	行  45: #include "php_crypt.h"
	行  46: #include "ext/random/php_random.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\crypt_blowfish.c （匹配3次）
	行  46: #include <string.h>
	行  48: #include <errno.h>
	行  54: #include "crypt_blowfish.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\crypt_freesec.c （匹配4次）
	行  61: #include <sys/types.h>
	行  62: #include <string.h>
	行  65: #include <stdio.h>
	行  68: #include "crypt_freesec.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\crypt_freesec.h （匹配1次）
	行  4: #include <stdint.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\crypt_sha256.c （匹配6次）
	行   5: #include "php.h"
	行   6: #include "php_main.h"
	行   8: #include <errno.h>
	行   9: #include <limits.h>
	行  20: #include <stdio.h>
	行  21: #include <stdlib.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\crypt_sha512.c （匹配6次）
	行   5: #include "php.h"
	行   6: #include "php_main.h"
	行   8: #include <errno.h>
	行   9: #include <limits.h>
	行  19: #include <stdio.h>
	行  20: #include <stdlib.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\css.c （匹配2次）
	行 17: #include "php.h"
	行 18: #include "info.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\datetime.c （匹配6次）
	行  19: #include "php.h"
	行  20: #include "zend_operators.h"
	行  21: #include "datetime.h"
	行  22: #include "php_globals.h"
	行  24: #include <time.h>
	行  28: #include <stdio.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\dir.c （匹配13次）
	行  19: #include "php.h"
	行  20: #include "fopen_wrappers.h"
	行  21: #include "file.h"
	行  22: #include "php_dir.h"
	行  23: #include "php_string.h"
	行  24: #include "php_scandir.h"
	行  25: #include "basic_functions.h"
	行  26: #include "dir_arginfo.h"
	行  29: #include <unistd.h>
	行  32: #include <errno.h>
	行  35: #include "win32/readdir.h"
	行  41: #include <glob.h>
	行  43: #include "win32/glob.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\dl.c （匹配12次）
	行  19: #include "php.h"
	行  20: #include "dl.h"
	行  21: #include "php_globals.h"
	行  22: #include "php_ini.h"
	行  23: #include "ext/standard/info.h"
	行  25: #include "SAPI.h"
	行  28: #include <stdlib.h>
	行  29: #include <stdio.h>
	行  30: #include <string.h>
	行  32: #include "win32/param.h"
	行  33: #include "win32/winutil.h"
	行  36: #include <sys/param.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\dns.c （匹配10次）
	行   20: #include "php.h"
	行   21: #include "php_network.h"
	行   24: #include <sys/socket.h>
	行   33: #include <netinet/in.h>
	行   35: #include <arpa/inet.h>
	行   37: #include <netdb.h>
	行   46: #include <arpa/nameser.h>
	行   49: #include <resolv.h>
	行   56: #include <dns.h>
	行   70: #include "php_dns.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\dns_win32.c （匹配5次）
	行  17: #include "php.h"
	行  19: #include <windows.h>
	行  20: #include <Winbase.h >
	行  21: #include <Windns.h>
	行  23: #include "php_dns.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\exec.c （匹配17次）
	行  18: #include <stdio.h>
	行  19: #include "php.h"
	行  20: #include <ctype.h>
	行  21: #include "php_string.h"
	行  22: #include "ext/standard/head.h"
	行  23: #include "ext/standard/file.h"
	行  24: #include "basic_functions.h"
	行  25: #include "exec.h"
	行  26: #include "php_globals.h"
	行  27: #include "SAPI.h"
	行  30: #include <sys/wait.h>
	行  33: #include <signal.h>
	行  36: #include <sys/types.h>
	行  39: #include <sys/stat.h>
	行  42: #include <fcntl.h>
	行  46: #include <unistd.h>
	行  49: #include <limits.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\file.c （匹配26次）
	行   24: #include "php.h"
	行   25: #include "php_globals.h"
	行   26: #include "ext/standard/flock_compat.h"
	行   27: #include "ext/standard/exec.h"
	行   28: #include "ext/standard/php_filestat.h"
	行   29: #include "php_open_temporary_file.h"
	行   30: #include "ext/standard/basic_functions.h"
	行   31: #include "php_ini.h"
	行   32: #include "zend_smart_str.h"
	行   34: #include <stdio.h>
	行   35: #include <stdlib.h>
	行   36: #include <errno.h>
	行   37: #include <wchar.h>
	行   38: #include <sys/types.h>
	行   39: #include <sys/stat.h>
	行   40: #include <fcntl.h>
	行   64: #include "ext/standard/head.h"
	行   65: #include "php_string.h"
	行   66: #include "file.h"
	行   80: #include "fsock.h"
	行   81: #include "fopen_wrappers.h"
	行   82: #include "streamsfuncs.h"
	行   83: #include "php_globals.h"
	行   97: #include "scanf.h"
	行   98: #include "zend_API.h"
	行  113: #include "file_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\file.h （匹配1次）
	行  20: #include "php_network.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\filestat.c （匹配12次）
	行   17: #include "php.h"
	行   18: #include "fopen_wrappers.h"
	行   19: #include "php_globals.h"
	行   21: #include <stdlib.h>
	行   22: #include <sys/stat.h>
	行   23: #include <string.h>
	行   24: #include <errno.h>
	行   25: #include <ctype.h>
	行   26: #include <time.h>
	行   84: #include "win32/winutil.h"
	行   87: #include "basic_functions.h"
	行   88: #include "php_filestat.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\filters.c （匹配6次）
	行   21: #include "php.h"
	行   22: #include "php_globals.h"
	行   23: #include "ext/standard/basic_functions.h"
	行   24: #include "ext/standard/file.h"
	行   25: #include "ext/standard/php_string.h"
	行   26: #include "zend_smart_str.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\flock_compat.c （匹配3次）
	行  17: #include "php.h"
	行  18: #include <errno.h>
	行  19: #include "ext/standard/flock_compat.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\flock_compat.h （匹配5次）
	行 21: #include <unistd.h>
	行 22: #include <fcntl.h>
	行 23: #include <sys/file.h>
	行 27: #include <io.h>
	行 28: #include "config.w32.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\formatted_print.c （匹配8次）
	行  17: #include <math.h>				/* modf() */
	行  18: #include "php.h"
	行  19: #include "ext/standard/head.h"
	行  20: #include "php_string.h"
	行  21: #include "zend_execute.h"
	行  22: #include <stdio.h>
	行  24: #include <locale.h>
	行  26: #include "ext/standard/php_string.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\fsock.c （匹配6次）
	行  19: #include "php.h"
	行  20: #include "php_globals.h"
	行  21: #include <stdlib.h>
	行  22: #include <stddef.h>
	行  23: #include "php_network.h"
	行  24: #include "file.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\fsock.h （匹配2次）
	行 22: #include "file.h"
	行 24: #include "php_network.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\ftok.c （匹配4次）
	行 17: #include "php.h"
	行 19: #include <sys/types.h>
	行 22: #include <sys/ipc.h>
	行 26: #include "win32/ipc.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\ftp_fopen_wrapper.c （匹配22次）
	行   20: #include "php.h"
	行   21: #include "php_globals.h"
	行   22: #include "php_network.h"
	行   23: #include "php_ini.h"
	行   25: #include <stdio.h>
	行   26: #include <stdlib.h>
	行   27: #include <errno.h>
	行   28: #include <sys/types.h>
	行   29: #include <sys/stat.h>
	行   30: #include <fcntl.h>
	行   33: #include <winsock2.h>
	行   35: #include "win32/param.h"
	行   37: #include <sys/param.h>
	行   40: #include "php_standard.h"
	行   42: #include <sys/types.h>
	行   44: #include <sys/socket.h>
	行   48: #include <winsock2.h>
	行   50: #include <netinet/in.h>
	行   51: #include <netdb.h>
	行   53: #include <arpa/inet.h>
	行   62: #include <sys/un.h>
	行   65: #include "php_fopen_wrappers.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\head.c （匹配10次）
	行  17: #include <stdio.h>
	行  18: #include "php.h"
	行  19: #include "ext/standard/php_standard.h"
	行  20: #include "ext/date/php_date.h"
	行  21: #include "SAPI.h"
	行  22: #include "php_main.h"
	行  23: #include "head.h"
	行  24: #include <time.h>
	行  26: #include "php_globals.h"
	行  27: #include "zend_smart_str.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\hrtime.c （匹配2次）
	行  18: #include "php.h"
	行  19: #include "hrtime.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\html.c （匹配10次）
	行   37: #include "php.h"
	行   39: #include "config.w32.h"
	行   41: #include <php_config.h>
	行   43: #include "php_standard.h"
	行   44: #include "php_string.h"
	行   45: #include "SAPI.h"
	行   46: #include <locale.h>
	行   48: #include <langinfo.h>
	行   51: #include <zend_hash.h>
	行   52: #include "html_tables.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\http.c （匹配3次）
	行  17: #include "php_http.h"
	行  18: #include "php_ini.h"
	行  19: #include "url.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\http_fopen_wrapper.c （匹配24次）
	行   21: #include "php.h"
	行   22: #include "php_globals.h"
	行   23: #include "php_streams.h"
	行   24: #include "php_network.h"
	行   25: #include "php_ini.h"
	行   26: #include "ext/standard/basic_functions.h"
	行   27: #include "zend_smart_str.h"
	行   29: #include <stdio.h>
	行   30: #include <stdlib.h>
	行   31: #include <errno.h>
	行   32: #include <sys/types.h>
	行   33: #include <sys/stat.h>
	行   34: #include <fcntl.h>
	行   38: #include "win32/param.h"
	行   40: #include <sys/param.h>
	行   43: #include "php_standard.h"
	行   45: #include <sys/types.h>
	行   47: #include <sys/socket.h>
	行   51: #include <winsock2.h>
	行   53: #include <netinet/in.h>
	行   54: #include <netdb.h>
	行   56: #include <arpa/inet.h>
	行   65: #include <sys/un.h>
	行   68: #include "php_fopen_wrappers.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\image.c （匹配9次）
	行   18: #include "php.h"
	行   19: #include <stdio.h>
	行   21: #include <fcntl.h>
	行   23: #include "fopen_wrappers.h"
	行   24: #include "ext/standard/fsock.h"
	行   25: #include "libavifinfo/avifinfo.h"
	行   27: #include <unistd.h>
	行   29: #include "php_image.h"
	行   32: #include "zlib.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\incomplete_class.c （匹配3次）
	行  17: #include "php.h"
	行  18: #include "basic_functions.h"
	行  19: #include "php_incomplete_class.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\info.c （匹配16次）
	行   19: #include "php.h"
	行   20: #include "php_ini.h"
	行   21: #include "php_globals.h"
	行   22: #include "ext/standard/head.h"
	行   23: #include "ext/standard/html.h"
	行   24: #include "info.h"
	行   25: #include "credits.h"
	行   26: #include "css.h"
	行   27: #include "SAPI.h"
	行   28: #include <time.h>
	行   29: #include "php_main.h"
	行   30: #include "zend_globals.h"		/* needs ELS */
	行   31: #include "zend_extensions.h"
	行   32: #include "zend_highlight.h"
	行   34: #include <sys/utsname.h>
	行   36: #include "url.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\iptc.c （匹配4次）
	行  31: #include "php.h"
	行  32: #include "ext/standard/head.h"
	行  34: #include <sys/stat.h>
	行  36: #include <stdint.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\levenshtein.c （匹配2次）
	行 17: #include "php.h"
	行 18: #include "php_string.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\libavifinfo\\avifinfo.c （匹配4次）
	行  10: #include "avifinfo.h"
	行  12: #include <stdint.h>
	行  13: #include <stdio.h>
	行  14: #include <string.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\libavifinfo\\avifinfo.h （匹配2次）
	行 13: #include <stddef.h>
	行 14: #include <stdint.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\link.c （匹配13次）
	行  17: #include "php.h"
	行  18: #include "php_filestat.h"
	行  19: #include "php_globals.h"
	行  24: #include <WinBase.h>
	行  27: #include <stdlib.h>
	行  29: #include <unistd.h>
	行  32: #include <sys/stat.h>
	行  34: #include <string.h>
	行  37: #include "win32/pwd.h"
	行  39: #include <pwd.h>
	行  45: #include <errno.h>
	行  46: #include <ctype.h>
	行  48: #include "php_string.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\mail.c （匹配15次）
	行  17: #include <stdlib.h>
	行  18: #include <ctype.h>
	行  19: #include <stdio.h>
	行  20: #include <time.h>
	行  21: #include "php.h"
	行  22: #include "ext/standard/info.h"
	行  23: #include "ext/standard/php_string.h"
	行  24: #include "ext/standard/basic_functions.h"
	行  25: #include "ext/date/php_date.h"
	行  26: #include "zend_smart_str.h"
	行  39: #include "php_syslog.h"
	行  40: #include "php_mail.h"
	行  41: #include "php_ini.h"
	行  42: #include "php_string.h"
	行  43: #include "exec.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\math.c （匹配10次）
	行   20: #include "php.h"
	行   21: #include "php_math.h"
	行   22: #include "zend_multiply.h"
	行   23: #include "zend_exceptions.h"
	行   24: #include "zend_portability.h"
	行   25: #include "zend_bitset.h"
	行   27: #include <math.h>
	行   28: #include <float.h>
	行   29: #include <stdlib.h>
	行   31: #include "basic_functions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\md5.c （匹配3次）
	行  19: #include "php.h"
	行  20: #include "md5.h"
	行 135: #include <string.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\md5.h （匹配1次）
	行 24: #include "ext/standard/basic_functions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\metaphone.c （匹配1次）
	行  21: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\microtime.c （匹配12次）
	行  17: #include "php.h"
	行  20: #include <sys/types.h>
	行  23: #include "win32/time.h"
	行  24: #include "win32/getrusage.h"
	行  26: #include <sys/time.h>
	行  29: #include <sys/resource.h>
	行  32: #include <unistd.h>
	行  34: #include <stdlib.h>
	行  35: #include <string.h>
	行  36: #include <stdio.h>
	行  37: #include <errno.h>
	行  39: #include "ext/date/php_date.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\net.c （匹配3次）
	行  17: #include "php.h"
	行  18: #include "php_network.h"
	行  32: #include <as400_protos.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\pack.c （匹配16次）
	行   17: #include "php.h"
	行   19: #include <stdio.h>
	行   20: #include <stdlib.h>
	行   21: #include <errno.h>
	行   22: #include <sys/types.h>
	行   23: #include <sys/stat.h>
	行   24: #include <fcntl.h>
	行   27: #include "win32/param.h"
	行   29: #include <sys/param.h>
	行   31: #include "ext/standard/head.h"
	行   32: #include "php_string.h"
	行   33: #include "pack.h"
	行   36: #include "win32/pwd.h"
	行   38: #include <pwd.h>
	行   41: #include "fsock.h"
	行   43: #include <netinet/in.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\pageinfo.c （匹配12次）
	行  17: #include "php.h"
	行  18: #include "pageinfo.h"
	行  19: #include "SAPI.h"
	行  21: #include <stdio.h>
	行  22: #include <stdlib.h>
	行  25: #include "win32/pwd.h"
	行  27: #include <pwd.h>
	行  40: #include <unistd.h>
	行  42: #include <sys/stat.h>
	行  43: #include <sys/types.h>
	行  45: #include <process.h>
	行  48: #include "ext/standard/basic_functions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\password.c （匹配11次）
	行  18: #include <stdlib.h>
	行  20: #include "php.h"
	行  22: #include "fcntl.h"
	行  23: #include "php_password.h"
	行  24: #include "php_crypt.h"
	行  25: #include "base64.h"
	行  26: #include "zend_interfaces.h"
	行  27: #include "info.h"
	行  28: #include "ext/random/php_random.h"
	行  30: #include "argon2.h"
	行  34: #include "win32/winutil.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\php_crypt_r.c （匹配5次）
	行  33: #include "php.h"
	行  35: #include <string.h>
	行  42: #include "php_crypt_r.h"
	行  43: #include "crypt_freesec.h"
	行  44: #include "ext/standard/md5.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\php_crypt_r.h （匹配2次）
	行 21: #include "crypt_freesec.h"
	行 32: #include "crypt_blowfish.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\php_ext_syslog.h （匹配1次）
	行 22: #include "php_syslog.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\php_fopen_wrapper.c （匹配8次）
	行  19: #include <stdio.h>
	行  20: #include <stdlib.h>
	行  25: #include "php.h"
	行  26: #include "php_globals.h"
	行  27: #include "php_standard.h"
	行  28: #include "php_memory_streams.h"
	行  29: #include "php_fopen_wrappers.h"
	行  30: #include "SAPI.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\php_http.h （匹配2次）
	行 20: #include "php.h"
	行 21: #include "zend_smart_str.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\php_incomplete_class.h （匹配1次）
	行 20: #include "ext/standard/basic_functions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\php_lcg.h （匹配1次）
	行 1: #include "ext/random/php_random.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\php_math.h （匹配1次）
	行  29: #include <math.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\php_mt_rand.h （匹配1次）
	行 1: #include "ext/random/php_random.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\php_net.h （匹配2次）
	行 20: #include "php.h"
	行 21: #include "php_network.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\php_rand.h （匹配1次）
	行 1: #include "ext/random/php_random.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\php_random.h （匹配1次）
	行 1: #include "ext/random/php_random.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\php_smart_string.h （匹配1次）
	行 19: #include "zend_smart_string.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\php_smart_string_public.h （匹配1次）
	行 19: #include "zend_smart_string_public.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\php_standard.h （匹配34次）
	行 17: #include "basic_functions.h"
	行 18: #include "php_math.h"
	行 19: #include "php_string.h"
	行 20: #include "base64.h"
	行 21: #include "php_dir.h"
	行 22: #include "php_dns.h"
	行 23: #include "php_mail.h"
	行 24: #include "md5.h"
	行 25: #include "sha1.h"
	行 26: #include "hrtime.h"
	行 27: #include "html.h"
	行 28: #include "exec.h"
	行 29: #include "file.h"
	行 30: #include "php_ext_syslog.h"
	行 31: #include "php_filestat.h"
	行 32: #include "php_browscap.h"
	行 33: #include "pack.h"
	行 34: #include "datetime.h"
	行 35: #include "url.h"
	行 36: #include "pageinfo.h"
	行 37: #include "fsock.h"
	行 38: #include "php_image.h"
	行 39: #include "info.h"
	行 40: #include "php_var.h"
	行 41: #include "quot_print.h"
	行 42: #include "dl.h"
	行 43: #include "php_crypt.h"
	行 44: #include "head.h"
	行 45: #include "php_output.h"
	行 46: #include "php_array.h"
	行 47: #include "php_assert.h"
	行 48: #include "php_versioning.h"
	行 49: #include "php_password.h"
	行 51: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\php_var.h （匹配2次）
	行 20: #include "ext/standard/basic_functions.h"
	行 21: #include "zend_smart_str_public.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\php_versioning.h （匹配1次）
	行 20: #include "ext/standard/basic_functions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\proc_open.c （匹配16次）
	行   17: #include "php.h"
	行   18: #include <stdio.h>
	行   19: #include <ctype.h>
	行   20: #include <signal.h>
	行   21: #include "php_string.h"
	行   22: #include "ext/standard/head.h"
	行   23: #include "ext/standard/basic_functions.h"
	行   24: #include "ext/standard/file.h"
	行   25: #include "exec.h"
	行   26: #include "php_globals.h"
	行   27: #include "SAPI.h"
	行   28: #include "main/php_network.h"
	行   29: #include "zend_smart_str.h"
	行   32: #include <sys/wait.h>
	行   36: #include <fcntl.h>
	行  124: #include "proc_open.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\quot_print.c （匹配7次）
	行  17: #include <stdlib.h>
	行  20: #include <unistd.h>
	行  22: #include <string.h>
	行  23: #include <errno.h>
	行  25: #include "php.h"
	行  26: #include "quot_print.h"
	行  28: #include <stdio.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\scanf.c （匹配12次）
	行   64: #include <stdio.h>
	行   65: #include <limits.h>
	行   66: #include <ctype.h>
	行   67: #include "php.h"
	行   68: #include "php_variables.h"
	行   69: #include <locale.h>
	行   70: #include "zend_execute.h"
	行   71: #include "zend_operators.h"
	行   72: #include "zend_strtod.h"
	行   73: #include "php_globals.h"
	行   74: #include "basic_functions.h"
	行   75: #include "scanf.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\sha1.c （匹配3次）
	行  17: #include "php.h"
	行  21: #include "sha1.h"
	行  22: #include "md5.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\sha1.h （匹配1次）
	行 20: #include "ext/standard/basic_functions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\soundex.c （匹配5次）
	行  17: #include "php.h"
	行  18: #include <stdlib.h>
	行  19: #include <errno.h>
	行  20: #include <ctype.h>
	行  21: #include "php_string.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\streamsfuncs.c （匹配15次）
	行   18: #include "php.h"
	行   19: #include "php_globals.h"
	行   20: #include "ext/standard/flock_compat.h"
	行   21: #include "ext/standard/file.h"
	行   22: #include "ext/standard/php_filestat.h"
	行   23: #include "php_open_temporary_file.h"
	行   24: #include "ext/standard/basic_functions.h"
	行   25: #include "php_ini.h"
	行   26: #include "streamsfuncs.h"
	行   27: #include "php_network.h"
	行   28: #include "php_string.h"
	行   30: #include <unistd.h>
	行   37: #include "win32/select.h"
	行   38: #include "win32/sockets.h"
	行   39: #include "win32/console.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\string.c （匹配19次）
	行   19: #include <stdio.h>
	行   20: #include "php.h"
	行   21: #include "php_string.h"
	行   22: #include "php_variables.h"
	行   23: #include <locale.h>
	行   32: #include "scanf.h"
	行   33: #include "zend_API.h"
	行   34: #include "zend_execute.h"
	行   35: #include "php_globals.h"
	行   36: #include "basic_functions.h"
	行   37: #include "zend_smart_str.h"
	行   38: #include <Zend/zend_exceptions.h>
	行   40: #include "TSRM.h"
	行   44: #include "ext/standard/file.h"
	行   46: #include "ext/standard/html.h"
	行   47: #include "ext/random/php_random.h"
	行   50: #include <emmintrin.h>
	行 3301: #include <tmmintrin.h>
	行 3303: #include <arm_neon.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\strnatcmp.c （匹配5次）
	行  25: #include <ctype.h>
	行  26: #include <string.h>
	行  27: #include <stdio.h>
	行  29: #include "php.h"
	行  30: #include "php_string.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\syslog.c （匹配10次）
	行  17: #include "php.h"
	行  20: #include "php_ini.h"
	行  21: #include "zend_globals.h"
	行  23: #include <stdlib.h>
	行  25: #include <unistd.h>
	行  28: #include <string.h>
	行  29: #include <errno.h>
	行  31: #include <stdio.h>
	行  32: #include "basic_functions.h"
	行  33: #include "php_ext_syslog.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\type.c （匹配2次）
	行  17: #include "php.h"
	行  18: #include "php_incomplete_class.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\uniqid.c （匹配9次）
	行 17: #include "php.h"
	行 19: #include <stdlib.h>
	行 21: #include <unistd.h>
	行 24: #include <string.h>
	行 25: #include <errno.h>
	行 27: #include <stdio.h>
	行 29: #include "win32/time.h"
	行 31: #include <sys/time.h>
	行 34: #include "ext/random/php_random.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\url.c （匹配8次）
	行  17: #include <stdlib.h>
	行  18: #include <string.h>
	行  19: #include <ctype.h>
	行  20: #include <sys/types.h>
	行  23: #include <emmintrin.h>
	行  26: #include "php.h"
	行  28: #include "url.h"
	行  29: #include "file.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\url_scanner_ex.h （匹配1次）
	行 34: #include "zend_smart_str_public.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\user_filters.c （匹配5次）
	行  19: #include "php.h"
	行  20: #include "php_globals.h"
	行  21: #include "ext/standard/basic_functions.h"
	行  22: #include "ext/standard/file.h"
	行  23: #include "ext/standard/user_filters_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\uuencode.c （匹配3次）
	行  53: #include <math.h>
	行  55: #include "php.h"
	行  56: #include "php_uuencode.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\var.c （匹配11次）
	行   20: #include <stdio.h>
	行   21: #include <stdlib.h>
	行   22: #include <errno.h>
	行   23: #include "php.h"
	行   24: #include "php_string.h"
	行   25: #include "php_var.h"
	行   26: #include "zend_smart_str.h"
	行   27: #include "basic_functions.h"
	行   28: #include "php_incomplete_class.h"
	行   29: #include "zend_enum.h"
	行   30: #include "zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\standard\\versioning.c （匹配7次）
	行  17: #include <stdio.h>
	行  18: #include <sys/types.h>
	行  19: #include <ctype.h>
	行  20: #include <stdlib.h>
	行  21: #include <string.h>
	行  22: #include "php.h"
	行  23: #include "php_versioning.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\sysvmsg\\php_sysvmsg.h （匹配1次）
	行 25: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\sysvmsg\\sysvmsg.c （匹配11次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  22: #include "php_globals.h"
	行  23: #include "ext/standard/info.h"
	行  24: #include "php_sysvmsg.h"
	行  25: #include "sysvmsg_arginfo.h"
	行  26: #include "ext/standard/php_var.h"
	行  27: #include "zend_smart_str.h"
	行  29: #include <sys/types.h>
	行  30: #include <sys/ipc.h>
	行  31: #include <sys/msg.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\sysvsem\\php_sysvsem.h （匹配1次）
	行 25: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\sysvsem\\sysvsem.c （匹配9次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  26: #include <sys/types.h>
	行  27: #include <sys/ipc.h>
	行  28: #include <sys/sem.h>
	行  29: #include <errno.h>
	行  31: #include "sysvsem_arginfo.h"
	行  32: #include "php_sysvsem.h"
	行  33: #include "ext/standard/info.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\sysvshm\\php_sysvshm.h （匹配2次）
	行 25: #include "php_version.h"
	行 28: #include <sys/types.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\sysvshm\\sysvshm.c （匹配9次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  25: #include <errno.h>
	行  27: #include "php_sysvshm.h"
	行  28: #include "sysvshm_arginfo.h"
	行  29: #include "ext/standard/info.h"
	行  30: #include "ext/standard/php_var.h"
	行  31: #include "zend_smart_str.h"
	行  32: #include "php_ini.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\tidy\\php_tidy.h （匹配1次）
	行 23: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\tidy\\tidy.c （匹配10次）
	行   18: #include "config.h"
	行   21: #include "php.h"
	行   22: #include "php_tidy.h"
	行   26: #include "php_ini.h"
	行   27: #include "ext/standard/info.h"
	行   30: #include "tidy.h"
	行   32: #include "tidyp.h"
	行   36: #include "tidybuffio.h"
	行   38: #include "buffio.h"
	行   41: #include "tidy_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\tokenizer\\php_tokenizer.h （匹配2次）
	行 23: #include "php_version.h"
	行 29: #include "TSRM.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\tokenizer\\tokenizer.c （匹配13次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  22: #include "php_ini.h"
	行  23: #include "ext/standard/info.h"
	行  24: #include "php_tokenizer.h"
	行  26: #include "zend.h"
	行  27: #include "zend_exceptions.h"
	行  28: #include "zend_language_scanner.h"
	行  29: #include "zend_language_scanner_defs.h"
	行  30: #include <zend_language_parser.h>
	行  31: #include "zend_interfaces.h"
	行  33: #include "tokenizer_data_arginfo.h"
	行  34: #include "tokenizer_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\tokenizer\\tokenizer_data.c （匹配3次）
	行  22: #include "php.h"
	行  23: #include "zend.h"
	行  24: #include <zend_language_parser.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\xml\\compat.c （匹配2次）
	行  17: #include "php.h"
	行  19: #include "expat_compat.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\xml\\expat_compat.h （匹配10次）
	行  21: #include "config.w32.h"
	行  23: #include <php_config.h>
	行  37: #include "php.h"
	行  38: #include "php_compat.h"
	行  40: #include <libxml/parser.h>
	行  41: #include <libxml/parserInternals.h>
	行  42: #include <libxml/tree.h>
	行  43: #include <libxml/hash.h>
	行 153: #include "php.h"
	行 154: #include <expat.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\xml\\php_xml.h （匹配2次）
	行 27: #include "php_version.h"
	行 30: #include "expat_compat.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\xml\\xml.c （匹配8次）
	行   20: #include "config.h"
	行   23: #include "php.h"
	行   25: #include "zend_variables.h"
	行   26: #include "ext/standard/info.h"
	行   27: #include "ext/standard/html.h"
	行   31: #include "php_xml.h"
	行   34: #include "ext/libxml/php_libxml.h"
	行   37: #include "xml_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\xmlreader\\php_xmlreader.c （匹配10次）
	行   18: #include "config.h"
	行   22: #include "php.h"
	行   23: #include "php_ini.h"
	行   24: #include "ext/standard/info.h"
	行   25: #include "php_xmlreader.h"
	行   27: #include "ext/dom/xml_common.h"
	行   28: #include "ext/dom/dom_ce.h"
	行   30: #include <libxml/xmlreader.h>
	行   31: #include <libxml/uri.h>
	行   32: #include "php_xmlreader_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\xmlreader\\php_xmlreader.h （匹配4次）
	行 23: #include "php_version.h"
	行 27: #include "TSRM.h"
	行 30: #include "ext/libxml/php_libxml.h"
	行 31: #include <libxml/xmlreader.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\xmlwriter\\php_xmlwriter.c （匹配7次）
	行   19: #include "config.h"
	行   23: #include "php.h"
	行   24: #include "php_ini.h"
	行   25: #include "ext/standard/info.h"
	行   26: #include "php_xmlwriter.h"
	行   27: #include "php_xmlwriter_arginfo.h"
	行   28: #include "ext/standard/php_string.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\xmlwriter\\php_xmlwriter.h （匹配5次）
	行 24: #include "php_version.h"
	行 28: #include "TSRM.h"
	行 31: #include <libxml/tree.h>
	行 32: #include <libxml/xmlwriter.h>
	行 33: #include <libxml/uri.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\xsl\\php_xsl.c （匹配6次）
	行  18: #include "config.h"
	行  21: #include "php.h"
	行  22: #include "php_ini.h"
	行  23: #include "ext/standard/info.h"
	行  24: #include "php_xsl.h"
	行  25: #include "php_xsl_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\xsl\\php_xsl.h （匹配12次）
	行 23: #include "php_version.h"
	行 27: #include "TSRM.h"
	行 30: #include <libxslt/xsltconfig.h>
	行 31: #include <libxslt/xsltInternals.h>
	行 32: #include <libxslt/xsltutils.h>
	行 33: #include <libxslt/transform.h>
	行 34: #include <libxslt/security.h>
	行 36: #include <libexslt/exslt.h>
	行 37: #include <libexslt/exsltconfig.h>
	行 40: #include "../dom/xml_common.h"
	行 42: #include <libxslt/extensions.h>
	行 43: #include <libxml/xpathInternals.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\xsl\\xsltprocessor.c （匹配4次）
	行  19: #include "config.h"
	行  22: #include "php.h"
	行  23: #include "php_xsl.h"
	行  24: #include "ext/libxml/php_libxml.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\zend_test\\fiber.c （匹配5次）
	行  17: #include "php_test.h"
	行  18: #include "fiber.h"
	行  19: #include "fiber_arginfo.h"
	行  20: #include "zend_fibers.h"
	行  21: #include "zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\zend_test\\fiber.h （匹配1次）
	行 20: #include "zend_fibers.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\zend_test\\observer.c （匹配6次）
	行  17: #include "php.h"
	行  18: #include "php_test.h"
	行  19: #include "observer.h"
	行  20: #include "zend_observer.h"
	行  21: #include "zend_smart_str.h"
	行  22: #include "ext/standard/php_var.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\zend_test\\php_test.h （匹配2次）
	行 20: #include "fiber.h"
	行 28: #include "TSRM.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\zend_test\\test.c （匹配13次）
	行  21: #include "php.h"
	行  22: #include "php_ini.h"
	行  23: #include "ext/standard/info.h"
	行  24: #include "php_test.h"
	行  25: #include "observer.h"
	行  26: #include "fiber.h"
	行  27: #include "zend_attributes.h"
	行  28: #include "zend_enum.h"
	行  29: #include "zend_interfaces.h"
	行  30: #include "zend_weakrefs.h"
	行  31: #include "Zend/Optimizer/zend_optimizer.h"
	行  32: #include "test.h"
	行  33: #include "test_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\zip\\php_zip.c （匹配14次）
	行   19: #include "config.h"
	行   22: #include "php.h"
	行   23: #include "php_ini.h"
	行   24: #include "ext/standard/info.h"
	行   25: #include "ext/standard/file.h"
	行   26: #include "ext/standard/php_string.h"
	行   27: #include "ext/pcre/php_pcre.h"
	行   28: #include "ext/standard/php_filestat.h"
	行   29: #include "zend_attributes.h"
	行   30: #include "zend_interfaces.h"
	行   31: #include "php_zip.h"
	行   32: #include "php_zip_arginfo.h"
	行   36: #include <glob.h>
	行   38: #include "win32/glob.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\zip\\php_zip.h （匹配2次）
	行 25: #include "TSRM.h"
	行 28: #include <zip.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\zip\\zip_stream.c （匹配8次）
	行  20: #include "php.h"
	行  23: #include "php_streams.h"
	行  24: #include "ext/standard/file.h"
	行  25: #include "ext/standard/php_string.h"
	行  26: #include "fopen_wrappers.h"
	行  27: #include "php_zip.h"
	行  29: #include "ext/standard/url.h"
	行  32: #include <sys/types.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\zlib\\php_zlib.h （匹配2次）
	行 22: #include "php_version.h"
	行 25: #include <zlib.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\zlib\\zlib.c （匹配8次）
	行   22: #include "config.h"
	行   25: #include "php.h"
	行   26: #include "SAPI.h"
	行   27: #include "php_ini.h"
	行   28: #include "ext/standard/info.h"
	行   29: #include "ext/standard/php_string.h"
	行   30: #include "php_zlib.h"
	行   31: #include "zlib_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\zlib\\zlib_filter.c （匹配2次）
	行  17: #include "php.h"
	行  18: #include "php_zlib.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\ext\\zlib\\zlib_fopen_wrapper.c （匹配4次）
	行  22: #include "php.h"
	行  23: #include "php_zlib.h"
	行  24: #include "fopen_wrappers.h"
	行  26: #include "main/php_network.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\explicit_bzero.c （匹配2次）
	行 17: #include "php.h"
	行 27: #include <string.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\fastcgi.c （匹配9次）
	行   17: #include "php.h"
	行   18: #include "php_network.h"
	行   20: #include <string.h>
	行   21: #include <stdlib.h>
	行   22: #include <stdio.h>
	行   23: #include <stdarg.h>
	行   24: #include <errno.h>
	行   32: #include <windows.h>
	行  131: #include "fastcgi.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\fopen_wrappers.c （匹配23次）
	行  19: #include "php.h"
	行  20: #include "php_globals.h"
	行  21: #include "SAPI.h"
	行  23: #include <stdio.h>
	行  24: #include <stdlib.h>
	行  25: #include <errno.h>
	行  26: #include <sys/types.h>
	行  27: #include <sys/stat.h>
	行  28: #include <fcntl.h>
	行  32: #include "win32/param.h"
	行  34: #include <sys/param.h>
	行  37: #include "ext/standard/head.h"
	行  38: #include "ext/standard/php_standard.h"
	行  39: #include "zend_compile.h"
	行  40: #include "php_network.h"
	行  43: #include <pwd.h>
	行  46: #include <sys/types.h>
	行  48: #include <sys/socket.h>
	行  52: #include <winsock2.h>
	行  54: #include <netinet/in.h>
	行  55: #include <netdb.h>
	行  57: #include <arpa/inet.h>
	行  66: #include <sys/un.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\fopen_wrappers.h （匹配2次）
	行 21: #include "php_globals.h"
	行 22: #include "php_ini.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\getopt.c （匹配5次）
	行  17: #include <stdio.h>
	行  18: #include <string.h>
	行  19: #include <assert.h>
	行  20: #include <stdlib.h>
	行  21: #include "php_getopt.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\internal_functions_win32.c （匹配44次）
	行  19: #include "php.h"
	行  20: #include "php_main.h"
	行  21: #include "zend_modules.h"
	行  22: #include "zend_compile.h"
	行  23: #include <stdarg.h>
	行  24: #include <stdlib.h>
	行  25: #include <stdio.h>
	行  27: #include "ext/standard/dl.h"
	行  28: #include "ext/standard/file.h"
	行  29: #include "ext/standard/fsock.h"
	行  30: #include "ext/standard/head.h"
	行  31: #include "ext/standard/pack.h"
	行  32: #include "ext/standard/php_browscap.h"
	行  33: #include "ext/standard/php_crypt.h"
	行  34: #include "ext/standard/php_dir.h"
	行  35: #include "ext/standard/php_filestat.h"
	行  36: #include "ext/standard/php_mail.h"
	行  37: #include "ext/standard/php_ext_syslog.h"
	行  38: #include "ext/standard/php_standard.h"
	行  39: #include "ext/standard/php_array.h"
	行  40: #include "ext/standard/php_assert.h"
	行  41: #include "ext/reflection/php_reflection.h"
	行  42: #include "ext/random/php_random.h"
	行  44: #include "ext/bcmath/php_bcmath.h"
	行  47: #include "ext/calendar/php_calendar.h"
	行  50: #include "ext/ctype/php_ctype.h"
	行  52: #include "ext/date/php_date.h"
	行  54: #include "ext/ftp/php_ftp.h"
	行  57: #include "ext/iconv/php_iconv.h"
	行  59: #include "ext/standard/reg.h"
	行  60: #include "ext/pcre/php_pcre.h"
	行  62: #include "ext/odbc/php_odbc.h"
	行  65: #include "ext/session/php_session.h"
	行  68: #include "ext/mbstring/mbstring.h"
	行  71: #include "ext/tokenizer/php_tokenizer.h"
	行  74: #include "ext/zlib/php_zlib.h"
	行  77: #include "ext/libxml/php_libxml.h"
	行  79: #include "ext/dom/php_dom.h"
	行  82: #include "ext/simplexml/php_simplexml.h"
	行  86: #include "ext/xml/php_xml.h"
	行  88: #include "ext/com_dotnet/php_com_dotnet.h"
	行  89: #include "ext/spl/php_spl.h"
	行  91: #include "ext/xmlreader/php_xmlreader.h"
	行  94: #include "ext/xmlwriter/php_xmlwriter.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\main.c （匹配46次）
	行   23: #include "php.h"
	行   24: #include <stdio.h>
	行   25: #include <fcntl.h>
	行   27: #include "win32/time.h"
	行   28: #include "win32/signal.h"
	行   29: #include "win32/php_win32_globals.h"
	行   30: #include "win32/winutil.h"
	行   31: #include <process.h>
	行   34: #include <sys/time.h>
	行   37: #include <unistd.h>
	行   40: #include <signal.h>
	行   41: #include <locale.h>
	行   42: #include "zend.h"
	行   43: #include "zend_types.h"
	行   44: #include "zend_extensions.h"
	行   45: #include "php_ini.h"
	行   46: #include "php_globals.h"
	行   47: #include "php_main.h"
	行   48: #include "php_syslog.h"
	行   49: #include "fopen_wrappers.h"
	行   50: #include "ext/standard/php_standard.h"
	行   51: #include "ext/date/php_date.h"
	行   52: #include "php_variables.h"
	行   53: #include "ext/standard/credits.h"
	行   55: #include <io.h>
	行   56: #include "win32/php_registry.h"
	行   57: #include "ext/standard/flock_compat.h"
	行   59: #include "php_syslog.h"
	行   60: #include "Zend/zend_exceptions.h"
	行   63: #include <sys/types.h>
	行   64: #include <sys/wait.h>
	行   67: #include "zend_compile.h"
	行   68: #include "zend_execute.h"
	行   69: #include "zend_highlight.h"
	行   70: #include "zend_extensions.h"
	行   71: #include "zend_ini.h"
	行   72: #include "zend_dtrace.h"
	行   73: #include "zend_observer.h"
	行   74: #include "zend_system_id.h"
	行   76: #include "php_content_types.h"
	行   77: #include "php_ticks.h"
	行   78: #include "php_streams.h"
	行   79: #include "php_open_temporary_file.h"
	行   81: #include "SAPI.h"
	行   82: #include "rfc1867.h"
	行   84: #include "ext/standard/html_tables.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\network.c （匹配16次）
	行   20: #include "php.h"
	行   22: #include <stddef.h>
	行   23: #include <errno.h>
	行   33: #include <sys/param.h>
	行   36: #include <sys/types.h>
	行   38: #include <sys/socket.h>
	行   42: #include <fcntl.h>
	行   46: #include <sys/select.h>
	行   49: #include <poll.h>
	行   51: #include <sys/poll.h>
	行   56: #include <netinet/in.h>
	行   57: #include <netdb.h>
	行   59: #include <arpa/inet.h>
	行   67: #include "php_network.h"
	行   74: #include <sys/un.h>
	行   77: #include "ext/standard/file.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\output.c （匹配6次）
	行   27: #include "php.h"
	行   28: #include "ext/standard/head.h"
	行   29: #include "ext/standard/url_scanner_ex.h"
	行   30: #include "SAPI.h"
	行   31: #include "zend_stack.h"
	行   32: #include "php_output.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php.h （匹配32次）
	行  22: #include <dmalloc.h>
	行  30: #include "php_version.h"
	行  31: #include "zend.h"
	行  32: #include "zend_sort.h"
	行  33: #include "php_compat.h"
	行  35: #include "zend_API.h"
	行 119: #include <assert.h>
	行 122: #include <unix.h>
	行 126: #include <alloca.h>
	行 130: #include <build-defs.h>
	行 210: #include <stdlib.h>
	行 211: #include <ctype.h>
	行 213: #include <unistd.h>
	行 216: #include <stdarg.h>
	行 218: #include "zend_hash.h"
	行 219: #include "zend_alloc.h"
	行 220: #include "zend_stack.h"
	行 221: #include <string.h>
	行 225: #include "win32/param.h"
	行 227: #include <pwd.h>
	行 228: #include <sys/param.h>
	行 232: #include <limits.h>
	行 257: #include "snprintf.h"
	行 259: #include "spprintf.h"
	行 306: #include "php_syslog.h"
	行 417: #include "main/php_output.h"
	行 420: #include "php_streams.h"
	行 421: #include "php_memory_streams.h"
	行 422: #include "fopen_wrappers.h"
	行 426: #include "zend_virtual_cwd.h"
	行 428: #include "zend_constants.h"
	行 435: #include "php_reentrancy.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_compat.h （匹配2次）
	行  21: #include "config.w32.h"
	行  23: #include <php_config.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_content_types.c （匹配4次）
	行 17: #include "php.h"
	行 18: #include "SAPI.h"
	行 19: #include "rfc1867.h"
	行 21: #include "php_content_types.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_getopt.h （匹配1次）
	行 20: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_globals.h （匹配1次）
	行  20: #include "zend_globals.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_ini.c （匹配14次）
	行  17: #include "php.h"
	行  18: #include "ext/standard/info.h"
	行  19: #include "zend_ini.h"
	行  20: #include "zend_ini_scanner.h"
	行  21: #include "php_ini.h"
	行  22: #include "ext/standard/dl.h"
	行  23: #include "zend_extensions.h"
	行  24: #include "zend_highlight.h"
	行  25: #include "SAPI.h"
	行  26: #include "php_main.h"
	行  27: #include "php_scandir.h"
	行  29: #include "win32/php_registry.h"
	行  30: #include "win32/winutil.h"
	行  34: #include <dirent.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_ini.h （匹配1次）
	行 20: #include "zend_ini.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_ini_builder.c （匹配3次）
	行 17: #include "php_ini_builder.h"
	行 19: #include <ctype.h>
	行 20: #include <string.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_ini_builder.h （匹配1次）
	行 20: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_main.h （匹配3次）
	行 21: #include "zend_globals.h"
	行 22: #include "php_globals.h"
	行 23: #include "SAPI.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_memory_streams.h （匹配1次）
	行 20: #include "php_streams.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_network.h （匹配5次）
	行  20: #include <php.h>
	行  78: #include <sys/socket.h>
	行  82: #include <netdb.h>
	行  94: #include <sys/time.h>
	行  97: #include <stddef.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_odbc_utils.c （匹配2次）
	行  18: #include "config.h"
	行  21: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_odbc_utils.h （匹配1次）
	行 17: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_open_temporary_file.c （匹配15次）
	行  17: #include "php.h"
	行  18: #include "php_open_temporary_file.h"
	行  20: #include <errno.h>
	行  21: #include <sys/types.h>
	行  22: #include <sys/stat.h>
	行  23: #include <fcntl.h>
	行  27: #include "win32/param.h"
	行  28: #include "win32/winutil.h"
	行  30: #include <sys/param.h>
	行  31: #include <sys/socket.h>
	行  32: #include <netinet/in.h>
	行  33: #include <netdb.h>
	行  35: #include <arpa/inet.h>
	行  39: #include <sys/time.h>
	行  43: #include <sys/file.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_reentrancy.h （匹配4次）
	行  20: #include "php.h"
	行  22: #include <sys/types.h>
	行  24: #include <dirent.h>
	行  26: #include <time.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_scandir.c （匹配9次）
	行  18: #include "php.h"
	行  19: #include "php_scandir.h"
	行  22: #include <sys/types.h>
	行  26: #include <dirent.h>
	行  32: #include "win32/param.h"
	行  33: #include "win32/readdir.h"
	行  36: #include <stdlib.h>
	行  37: #include <search.h>
	行  43: #include <string.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_scandir.h （匹配5次）
	行 21: #include <sys/types.h>
	行 24: #include "config.w32.h"
	行 25: #include "win32/readdir.h"
	行 27: #include <php_config.h>
	行 31: #include <dirent.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_streams.h （匹配12次）
	行  21: #include <sys/time.h>
	行  23: #include <sys/types.h>
	行  24: #include <sys/stat.h>
	行  25: #include "zend.h"
	行  26: #include "zend_stream.h"
	行 101: #include "streams/php_stream_context.h"
	行 102: #include "streams/php_stream_filter_api.h"
	行 486: #include "streams/php_stream_transport.h"
	行 487: #include "streams/php_stream_plain_wrapper.h"
	行 488: #include "streams/php_stream_glob_wrapper.h"
	行 489: #include "streams/php_stream_userspace.h"
	行 490: #include "streams/php_stream_mmap.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_syslog.c （匹配7次）
	行  17: #include <stdio.h>
	行  18: #include <string.h>
	行  19: #include <stdlib.h>
	行  20: #include "php.h"
	行  21: #include "php_syslog.h"
	行  23: #include "zend.h"
	行  24: #include "zend_smart_string.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_syslog.h （匹配4次）
	行 20: #include "php.h"
	行 23: #include "win32/syslog.h"
	行 25: #include <php_config.h>
	行 27: #include <syslog.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_ticks.c （匹配2次）
	行 17: #include "php.h"
	行 18: #include "php_ticks.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_variables.c （匹配10次）
	行  18: #include <stdio.h>
	行  19: #include "php.h"
	行  20: #include "ext/standard/php_standard.h"
	行  21: #include "ext/standard/credits.h"
	行  22: #include "zend_smart_str.h"
	行  23: #include "php_variables.h"
	行  24: #include "php_globals.h"
	行  25: #include "php_content_types.h"
	行  26: #include "SAPI.h"
	行  27: #include "zend_globals.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\php_variables.h （匹配2次）
	行 21: #include "php.h"
	行 22: #include "SAPI.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\reentrancy.c （匹配8次）
	行  17: #include <sys/types.h>
	行  18: #include <string.h>
	行  19: #include <errno.h>
	行  21: #include <dirent.h>
	行  24: #include "php_reentrancy.h"
	行  25: #include "ext/random/php_random.h"                   /* for PHP_RAND_MAX */
	行  37: #include <TSRM.h>
	行 261: #include <stddef.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\rfc1867.c （匹配8次）
	行   24: #include <stdio.h>
	行   25: #include "php.h"
	行   26: #include "php_open_temporary_file.h"
	行   27: #include "zend_globals.h"
	行   28: #include "php_globals.h"
	行   29: #include "php_variables.h"
	行   30: #include "rfc1867.h"
	行   31: #include "zend_smart_string.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\rfc1867.h （匹配1次）
	行 20: #include "SAPI.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\safe_bcmp.c （匹配2次）
	行 17: #include "php.h"
	行 19: #include <string.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\SAPI.c （匹配14次）
	行   19: #include <ctype.h>
	行   20: #include <sys/stat.h>
	行   22: #include "php.h"
	行   23: #include "SAPI.h"
	行   24: #include "php_variables.h"
	行   25: #include "php_ini.h"
	行   26: #include "ext/standard/php_string.h"
	行   27: #include "ext/standard/pageinfo.h"
	行   28: #include "ext/pcre/php_pcre.h"
	行   30: #include "TSRM.h"
	行   33: #include <sys/time.h>
	行   35: #include "win32/time.h"
	行   38: #include "rfc1867.h"
	行   40: #include "php_content_types.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\SAPI.h （匹配6次）
	行  20: #include "php.h"
	行  21: #include "zend.h"
	行  22: #include "zend_API.h"
	行  23: #include "zend_llist.h"
	行  24: #include "zend_operators.h"
	行  25: #include <sys/stat.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\snprintf.c （匹配13次）
	行   20: #include "php.h"
	行   22: #include <zend_strtod.h>
	行   24: #include <stddef.h>
	行   25: #include <stdio.h>
	行   26: #include <ctype.h>
	行   27: #include <sys/types.h>
	行   28: #include <stdarg.h>
	行   29: #include <string.h>
	行   30: #include <stdlib.h>
	行   31: #include <math.h>
	行   32: #include <inttypes.h>
	行   34: #include <locale.h>
	行   36: #include "ext/standard/php_string.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\snprintf.h （匹配1次）
	行  69: #include <stdbool.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\spprintf.c （匹配15次）
	行  78: #include "php.h"
	行  80: #include <stddef.h>
	行  81: #include <stdio.h>
	行  82: #include <ctype.h>
	行  83: #include <sys/types.h>
	行  84: #include <stdarg.h>
	行  85: #include <string.h>
	行  86: #include <stdlib.h>
	行  87: #include <math.h>
	行  88: #include <inttypes.h>
	行  90: #include <locale.h>
	行  92: #include "ext/standard/php_string.h"
	行  98: #include "snprintf.h"
	行 109: #include "zend_smart_str.h"
	行 110: #include "zend_smart_string.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\spprintf.h （匹配3次）
	行 20: #include "snprintf.h"
	行 21: #include "zend_smart_str_public.h"
	行 22: #include "zend_smart_string_public.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\streams\\cast.c （匹配8次）
	行  20: #include "php.h"
	行  21: #include "php_globals.h"
	行  22: #include "php_network.h"
	行  23: #include "php_open_temporary_file.h"
	行  24: #include "ext/standard/file.h"
	行  25: #include <stddef.h>
	行  26: #include <fcntl.h>
	行  28: #include "php_streams_int.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\streams\\filter.c （匹配8次）
	行  17: #include "php.h"
	行  18: #include "php_globals.h"
	行  19: #include "php_network.h"
	行  20: #include "php_open_temporary_file.h"
	行  21: #include "ext/standard/file.h"
	行  22: #include <stddef.h>
	行  23: #include <fcntl.h>
	行  25: #include "php_streams_int.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\streams\\glob_wrapper.c （匹配2次）
	行  17: #include "php.h"
	行  18: #include "php_streams_int.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\streams\\memory.c （匹配2次）
	行  20: #include "php.h"
	行  21: #include "ext/standard/base64.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\streams\\mmap.c （匹配2次）
	行 18: #include "php.h"
	行 19: #include "php_streams_int.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\streams\\php_stream_transport.h （匹配2次）
	行  18: #include "config.w32.h"
	行  19: #include <Ws2tcpip.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\streams\\plain_wrapper.c （匹配14次）
	行   17: #include "php.h"
	行   18: #include "php_globals.h"
	行   19: #include "php_network.h"
	行   20: #include "php_open_temporary_file.h"
	行   21: #include "ext/standard/file.h"
	行   22: #include "ext/standard/flock_compat.h"
	行   23: #include "ext/standard/php_filestat.h"
	行   24: #include <stddef.h>
	行   25: #include <fcntl.h>
	行   27: #include <sys/wait.h>
	行   30: #include <sys/file.h>
	行   33: #include <sys/mman.h>
	行   35: #include "SAPI.h"
	行   37: #include "php_streams_int.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\streams\\streams.c （匹配11次）
	行   23: #include "php.h"
	行   24: #include "php_globals.h"
	行   25: #include "php_memory_streams.h"
	行   26: #include "php_network.h"
	行   27: #include "php_open_temporary_file.h"
	行   28: #include "ext/standard/file.h"
	行   29: #include "ext/standard/basic_functions.h" /* for BG(CurrentStatFile) */
	行   30: #include "ext/standard/php_string.h" /* for php_memnstr, used by php_stream_get_record() */
	行   31: #include <stddef.h>
	行   32: #include <fcntl.h>
	行   33: #include "php_streams_int.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\streams\\transports.c （匹配3次）
	行  17: #include "php.h"
	行  18: #include "php_streams_int.h"
	行  19: #include "ext/standard/file.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\streams\\userspace.c （匹配6次）
	行   18: #include "php.h"
	行   19: #include "php_globals.h"
	行   20: #include "ext/standard/file.h"
	行   21: #include "ext/standard/flock_compat.h"
	行   23: #include <sys/file.h>
	行   25: #include <stddef.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\streams\\xp_socket.c （匹配5次）
	行  17: #include "php.h"
	行  18: #include "ext/standard/file.h"
	行  19: #include "streams/php_streams_int.h"
	行  20: #include "php_network.h"
	行  27: #include <sys/un.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\strlcat.c （匹配3次）
	行 17: #include "php.h"
	行 54: #include <sys/types.h>
	行 55: #include <string.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\main\\strlcpy.c （匹配3次）
	行 17: #include "php.h"
	行 54: #include <sys/types.h>
	行 55: #include <string.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\apache2handler\\apache_config.c （匹配15次）
	行  19: #include "php.h"
	行  26: #include "php_ini.h"
	行  27: #include "php_apache.h"
	行  29: #include "apr_strings.h"
	行  30: #include "ap_config.h"
	行  31: #include "util_filter.h"
	行  32: #include "httpd.h"
	行  33: #include "http_config.h"
	行  34: #include "http_request.h"
	行  35: #include "http_core.h"
	行  36: #include "http_protocol.h"
	行  37: #include "http_log.h"
	行  38: #include "http_main.h"
	行  39: #include "util_script.h"
	行  40: #include "http_core.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\apache2handler\\mod_php.c （匹配2次）
	行 21: #include "php.h"
	行 28: #include "php_apache.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\apache2handler\\php_apache.h （匹配6次）
	行 20: #include "httpd.h"
	行 21: #include "http_config.h"
	行 22: #include "http_core.h"
	行 23: #include "http_log.h"
	行 25: #include "php.h"
	行 26: #include "main/php_streams.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\apache2handler\\php_functions.c （匹配23次）
	行  19: #include "php.h"
	行  26: #include "zend_smart_str.h"
	行  27: #include "ext/standard/info.h"
	行  28: #include "ext/standard/head.h"
	行  29: #include "php_ini.h"
	行  30: #include "SAPI.h"
	行  33: #include "apr_strings.h"
	行  34: #include "apr_time.h"
	行  35: #include "ap_config.h"
	行  36: #include "util_filter.h"
	行  37: #include "httpd.h"
	行  38: #include "http_config.h"
	行  39: #include "http_request.h"
	行  40: #include "http_core.h"
	行  41: #include "http_protocol.h"
	行  42: #include "http_log.h"
	行  43: #include "http_main.h"
	行  44: #include "util_script.h"
	行  45: #include "http_core.h"
	行  46: #include "ap_mpm.h"
	行  48: #include "unixd.h"
	行  51: #include "php_apache.h"
	行  52: #include "php_functions_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\apache2handler\\sapi_apache2.c （匹配22次）
	行  21: #include "php.h"
	行  28: #include "php_main.h"
	行  29: #include "php_ini.h"
	行  30: #include "php_variables.h"
	行  31: #include "SAPI.h"
	行  33: #include <fcntl.h>
	行  35: #include "zend_smart_str.h"
	行  36: #include "ext/standard/php_standard.h"
	行  38: #include "apr_strings.h"
	行  39: #include "ap_config.h"
	行  40: #include "util_filter.h"
	行  41: #include "httpd.h"
	行  42: #include "http_config.h"
	行  43: #include "http_request.h"
	行  44: #include "http_core.h"
	行  45: #include "http_protocol.h"
	行  46: #include "http_log.h"
	行  47: #include "http_main.h"
	行  48: #include "util_script.h"
	行  49: #include "http_core.h"
	行  50: #include "ap_mpm.h"
	行  52: #include "php_apache.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\cgi\\cgi_main.c （匹配25次）
	行   22: #include "php.h"
	行   23: #include "php_globals.h"
	行   24: #include "php_variables.h"
	行   25: #include "php_ini_builder.h"
	行   26: #include "zend_modules.h"
	行   28: #include "SAPI.h"
	行   30: #include <stdio.h>
	行   47: #include <signal.h>
	行   49: #include <locale.h>
	行   59: #include "zend.h"
	行   60: #include "zend_extensions.h"
	行   61: #include "php_ini.h"
	行   62: #include "php_globals.h"
	行   63: #include "php_main.h"
	行   64: #include "fopen_wrappers.h"
	行   65: #include "http_status_codes.h"
	行   66: #include "ext/standard/php_standard.h"
	行   67: #include "ext/standard/dl_arginfo.h"
	行   68: #include "ext/standard/url.h"
	行   81: #include "zend_compile.h"
	行   82: #include "zend_execute.h"
	行   83: #include "zend_highlight.h"
	行   85: #include "php_getopt.h"
	行   87: #include "fastcgi.h"
	行   88: #include "cgi_main_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\cli\\cli_win32.c （匹配1次）
	行 2: #include "php_cli.c"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\cli\\php_cli.c （匹配42次）
	行   21: #include "php.h"
	行   22: #include "php_globals.h"
	行   23: #include "php_variables.h"
	行   24: #include "php_ini_builder.h"
	行   25: #include "zend_hash.h"
	行   26: #include "zend_modules.h"
	行   27: #include "zend_interfaces.h"
	行   29: #include "ext/reflection/php_reflection.h"
	行   31: #include "SAPI.h"
	行   33: #include <stdio.h>
	行   34: #include "php.h"
	行   36: #include "win32/time.h"
	行   37: #include "win32/signal.h"
	行   38: #include "win32/console.h"
	行   39: #include <process.h>
	行   40: #include <shellapi.h>
	行   43: #include <sys/time.h>
	行   46: #include <unistd.h>
	行   49: #include <signal.h>
	行   50: #include <locale.h>
	行   51: #include "zend.h"
	行   52: #include "zend_extensions.h"
	行   53: #include "php_ini.h"
	行   54: #include "php_globals.h"
	行   55: #include "php_main.h"
	行   56: #include "fopen_wrappers.h"
	行   57: #include "ext/standard/php_standard.h"
	行   58: #include "ext/standard/dl_arginfo.h"
	行   59: #include "cli.h"
	行   61: #include <io.h>
	行   62: #include <fcntl.h>
	行   63: #include "win32/php_registry.h"
	行   67: #include <unixlib/local.h>
	行   70: #include "zend_compile.h"
	行   71: #include "zend_execute.h"
	行   72: #include "zend_highlight.h"
	行   73: #include "zend_exceptions.h"
	行   75: #include "php_getopt.h"
	行   78: #include "php_cli_server.h"
	行   81: #include "ps_title.h"
	行   82: #include "php_cli_process_title.h"
	行   83: #include "php_cli_process_title_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\cli\\php_cli_process_title.c （匹配4次）
	行 18: #include "config.h"
	行 21: #include "php.h"
	行 22: #include "php_cli_process_title.h"
	行 23: #include "ps_title.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\cli\\php_cli_server.c （匹配38次）
	行   18: #include <stdio.h>
	行   19: #include <stdlib.h>
	行   20: #include <fcntl.h>
	行   21: #include <assert.h>
	行   35: #include <unixlib/local.h>
	行   39: #include <sys/time.h>
	行   42: #include <unistd.h>
	行   45: #include <signal.h>
	行   46: #include <locale.h>
	行   49: #include <dlfcn.h>
	行   52: #include "SAPI.h"
	行   53: #include "php.h"
	行   54: #include "php_ini.h"
	行   55: #include "php_main.h"
	行   56: #include "php_globals.h"
	行   57: #include "php_variables.h"
	行   58: #include "zend_hash.h"
	行   59: #include "zend_modules.h"
	行   60: #include "fopen_wrappers.h"
	行   61: #include "http_status_codes.h"
	行   63: #include "zend_compile.h"
	行   64: #include "zend_execute.h"
	行   65: #include "zend_highlight.h"
	行   66: #include "zend_exceptions.h"
	行   68: #include "php_getopt.h"
	行   84: #include "ext/standard/file.h" /* for php_set_sock_blocking() :-( */
	行   85: #include "zend_smart_str.h"
	行   86: #include "ext/standard/html.h"
	行   87: #include "ext/standard/url.h" /* for php_raw_url_decode() */
	行   88: #include "ext/standard/php_string.h" /* for php_dirname() */
	行   89: #include "ext/date/php_date.h" /* for php_format_date() */
	行   90: #include "php_network.h"
	行   92: #include "php_http_parser.h"
	行   93: #include "php_cli_server.h"
	行   94: #include "php_cli_server_arginfo.h"
	行   95: #include "mime_type_map.h"
	行   97: #include "php_cli_process_title.h"
	行   98: #include "php_cli_process_title_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\cli\\php_cli_server.h （匹配1次）
	行 20: #include "SAPI.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\cli\\php_http_parser.c （匹配3次）
	行   21: #include <assert.h>
	行   22: #include <stddef.h>
	行   23: #include "php_http_parser.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\cli\\php_http_parser.h （匹配2次）
	行  29: #include <sys/types.h>
	行  37: #include <stdint.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\cli\\ps_title.c （匹配16次）
	行  32: #include "config.w32.h"
	行  33: #include <windows.h>
	行  34: #include <process.h>
	行  35: #include "win32/codepage.h"
	行  37: #include "php_config.h"
	行  41: #include "ps_title.h"
	行  42: #include <stdio.h>
	行  45: #include <sys/types.h>
	行  46: #include <unistd.h>
	行  49: #include <stdbool.h>
	行  50: #include <string.h>
	行  51: #include <stdlib.h>
	行  54: #include <sys/pstat.h> /* for HP-UX */
	行  57: #include <machine/vmparam.h> /* for old BSD */
	行  58: #include <sys/exec.h>
	行  61: #include <crt_externs.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\embed\\php_embed.c （匹配5次）
	行  17: #include "php_embed.h"
	行  18: #include "ext/standard/php_standard.h"
	行  19: #include "ext/standard/dl_arginfo.h"
	行  22: #include <io.h>
	行  23: #include <fcntl.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\embed\\php_embed.h （匹配6次）
	行 20: #include <main/php.h>
	行 21: #include <main/SAPI.h>
	行 22: #include <main/php_main.h>
	行 23: #include <main/php_variables.h>
	行 24: #include <main/php_ini.h>
	行 25: #include <zend_ini.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\events\\devpoll.c （匹配10次）
	行  17: #include "../fpm_config.h"
	行  18: #include "../fpm_events.h"
	行  19: #include "../fpm.h"
	行  20: #include "../zlog.h"
	行  24: #include <sys/types.h>
	行  25: #include <sys/stat.h>
	行  26: #include <fcntl.h>
	行  27: #include <poll.h>
	行  28: #include <sys/devpoll.h>
	行  29: #include <errno.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\events\\devpoll.h （匹配2次）
	行 20: #include "../fpm_config.h"
	行 21: #include "../fpm_events.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\events\\epoll.c （匹配6次）
	行  17: #include "../fpm_config.h"
	行  18: #include "../fpm_events.h"
	行  19: #include "../fpm.h"
	行  20: #include "../zlog.h"
	行  24: #include <sys/epoll.h>
	行  25: #include <errno.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\events\\epoll.h （匹配2次）
	行 20: #include "../fpm_config.h"
	行 21: #include "../fpm_events.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\events\\kqueue.c （匹配8次）
	行  17: #include "../fpm_config.h"
	行  18: #include "../fpm_events.h"
	行  19: #include "../fpm.h"
	行  20: #include "../zlog.h"
	行  24: #include <sys/types.h>
	行  25: #include <sys/event.h>
	行  26: #include <sys/time.h>
	行  28: #include <errno.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\events\\kqueue.h （匹配2次）
	行 20: #include "../fpm_config.h"
	行 21: #include "../fpm_events.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\events\\poll.c （匹配7次）
	行  17: #include "../fpm_config.h"
	行  18: #include "../fpm_events.h"
	行  19: #include "../fpm.h"
	行  20: #include "../zlog.h"
	行  24: #include <poll.h>
	行  25: #include <errno.h>
	行  26: #include <string.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\events\\poll.h （匹配2次）
	行 20: #include "../fpm_config.h"
	行 21: #include "../fpm_events.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\events\\port.c （匹配7次）
	行  17: #include "../fpm_config.h"
	行  18: #include "../fpm_events.h"
	行  19: #include "../fpm.h"
	行  20: #include "../zlog.h"
	行  24: #include <port.h>
	行  25: #include <poll.h>
	行  26: #include <errno.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\events\\port.h （匹配2次）
	行 20: #include "../fpm_config.h"
	行 21: #include "../fpm_events.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\events\\select.c （匹配9次）
	行  17: #include "../fpm_config.h"
	行  18: #include "../fpm_events.h"
	行  19: #include "../fpm.h"
	行  20: #include "../zlog.h"
	行  25: #include <sys/select.h>
	行  28: #include <sys/time.h>
	行  29: #include <sys/types.h>
	行  30: #include <unistd.h>
	行  32: #include <errno.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\events\\select.h （匹配2次）
	行 20: #include "../fpm_config.h"
	行 21: #include "../fpm_events.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm.c （匹配18次）
	行   3: #include "fpm_config.h"
	行   5: #include <stdlib.h> /* for exit */
	行   7: #include "fpm.h"
	行   8: #include "fpm_children.h"
	行   9: #include "fpm_signals.h"
	行  10: #include "fpm_env.h"
	行  11: #include "fpm_events.h"
	行  12: #include "fpm_cleanup.h"
	行  13: #include "fpm_php.h"
	行  14: #include "fpm_sockets.h"
	行  15: #include "fpm_unix.h"
	行  16: #include "fpm_process_ctl.h"
	行  17: #include "fpm_conf.h"
	行  18: #include "fpm_worker_pool.h"
	行  19: #include "fpm_scoreboard.h"
	行  20: #include "fpm_stdio.h"
	行  21: #include "fpm_log.h"
	行  22: #include "zlog.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm.h （匹配2次）
	行  6: #include <unistd.h>
	行  9: #include <sysexits.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_arrays.h （匹配3次）
	行   6: #include <stdlib.h>
	行   7: #include <string.h>
	行   9: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_atomic.h （匹配2次）
	行   6: #include <inttypes.h>
	行   7: #include <sched.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_children.c （匹配25次）
	行   3: #include "fpm_config.h"
	行   5: #include <sys/types.h>
	行   6: #include <sys/wait.h>
	行   7: #include <time.h>
	行   8: #include <unistd.h>
	行   9: #include <string.h>
	行  10: #include <stdio.h>
	行  12: #include "fpm.h"
	行  13: #include "fpm_children.h"
	行  14: #include "fpm_signals.h"
	行  15: #include "fpm_worker_pool.h"
	行  16: #include "fpm_sockets.h"
	行  17: #include "fpm_process_ctl.h"
	行  18: #include "fpm_php.h"
	行  19: #include "fpm_conf.h"
	行  20: #include "fpm_cleanup.h"
	行  21: #include "fpm_events.h"
	行  22: #include "fpm_clock.h"
	行  23: #include "fpm_stdio.h"
	行  24: #include "fpm_unix.h"
	行  25: #include "fpm_env.h"
	行  26: #include "fpm_scoreboard.h"
	行  27: #include "fpm_status.h"
	行  28: #include "fpm_log.h"
	行  30: #include "zlog.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_children.h （匹配5次）
	行  6: #include <sys/time.h>
	行  7: #include <sys/types.h>
	行  9: #include "fpm_worker_pool.h"
	行 10: #include "fpm_events.h"
	行 11: #include "zlog.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_cleanup.c （匹配4次）
	行  3: #include "fpm_config.h"
	行  5: #include <stdlib.h>
	行  7: #include "fpm_arrays.h"
	行  8: #include "fpm_cleanup.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_clock.c （匹配7次）
	行   3: #include "fpm_config.h"
	行   6: #include <time.h> /* for CLOCK_MONOTONIC */
	行   9: #include "fpm_clock.h"
	行  10: #include "zlog.h"
	行  53: #include <mach/mach.h>
	行  54: #include <mach/clock.h>
	行  55: #include <mach/mach_error.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_clock.h （匹配1次）
	行  6: #include <sys/time.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_conf.c （匹配30次）
	行    3: #include "fpm_config.h"
	行    5: #include <sys/types.h>
	行    6: #include <sys/stat.h>
	行    7: #include <fcntl.h>
	行    8: #include <string.h>
	行    9: #include <stdlib.h>
	行   10: #include <stddef.h>
	行   11: #include <string.h>
	行   12: #include <inttypes.h>
	行   17: #include <stdio.h>
	行   18: #include <unistd.h>
	行   20: #include "php.h"
	行   21: #include "zend_ini_scanner.h"
	行   22: #include "zend_globals.h"
	行   23: #include "zend_stream.h"
	行   24: #include "php_syslog.h"
	行   26: #include "fpm.h"
	行   27: #include "fpm_conf.h"
	行   28: #include "fpm_stdio.h"
	行   29: #include "fpm_worker_pool.h"
	行   30: #include "fpm_cleanup.h"
	行   31: #include "fpm_php.h"
	行   32: #include "fpm_sockets.h"
	行   33: #include "fpm_shm.h"
	行   34: #include "fpm_status.h"
	行   35: #include "fpm_log.h"
	行   36: #include "fpm_events.h"
	行   37: #include "fpm_unix.h"
	行   38: #include "zlog.h"
	行   40: #include "fpm_systemd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_conf.h （匹配2次）
	行   6: #include <stdint.h>
	行   7: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_config.h （匹配1次）
	行  3: #include <php_config.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_env.c （匹配8次）
	行   3: #include "fpm_config.h"
	行   6: #include <alloca.h>
	行   8: #include <stdio.h>
	行   9: #include <stdlib.h>
	行  10: #include <string.h>
	行  12: #include "fpm_env.h"
	行  13: #include "fpm.h"
	行  14: #include "fpm_cleanup.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_env.h （匹配1次）
	行  6: #include "fpm_worker_pool.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_events.c （匹配23次）
	行   3: #include "fpm_config.h"
	行   5: #include <unistd.h>
	行   6: #include <errno.h>
	行   7: #include <stdlib.h> /* for putenv */
	行   8: #include <string.h>
	行  10: #include <php.h>
	行  12: #include "fpm.h"
	行  13: #include "fpm_process_ctl.h"
	行  14: #include "fpm_events.h"
	行  15: #include "fpm_cleanup.h"
	行  16: #include "fpm_stdio.h"
	行  17: #include "fpm_signals.h"
	行  18: #include "fpm_children.h"
	行  19: #include "zlog.h"
	行  20: #include "fpm_clock.h"
	行  21: #include "fpm_log.h"
	行  23: #include "events/select.h"
	行  24: #include "events/poll.h"
	行  25: #include "events/epoll.h"
	行  26: #include "events/devpoll.h"
	行  27: #include "events/port.h"
	行  28: #include "events/kqueue.h"
	行  31: #include "fpm_systemd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_log.c （匹配14次）
	行   3: #include "php.h"
	行   4: #include "SAPI.h"
	行   5: #include <stdio.h>
	行   6: #include <unistd.h>
	行   7: #include <fcntl.h>
	行  10: #include <sys/times.h>
	行  13: #include "fpm_config.h"
	行  14: #include "fpm_log.h"
	行  15: #include "fpm_clock.h"
	行  16: #include "fpm_process_ctl.h"
	行  17: #include "fpm_signals.h"
	行  18: #include "fpm_scoreboard.h"
	行  19: #include "fastcgi.h"
	行  20: #include "zlog.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_log.h （匹配1次）
	行  5: #include "fpm_worker_pool.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_main.c （匹配38次）
	行   22: #include "php.h"
	行   23: #include "php_globals.h"
	行   24: #include "php_variables.h"
	行   25: #include "php_ini_builder.h"
	行   26: #include "zend_modules.h"
	行   27: #include "php.h"
	行   28: #include "zend_ini_scanner.h"
	行   29: #include "zend_globals.h"
	行   30: #include "zend_stream.h"
	行   32: #include "SAPI.h"
	行   34: #include <stdio.h>
	行   35: #include "php.h"
	行   45: #include <signal.h>
	行   47: #include <locale.h>
	行   61: #include "zend.h"
	行   62: #include "zend_extensions.h"
	行   63: #include "php_ini.h"
	行   64: #include "php_globals.h"
	行   65: #include "php_main.h"
	行   66: #include "fopen_wrappers.h"
	行   67: #include "ext/standard/php_standard.h"
	行   74: #include "zend_compile.h"
	行   75: #include "zend_execute.h"
	行   76: #include "zend_highlight.h"
	行   78: #include "php_getopt.h"
	行   80: #include "http_status_codes.h"
	行   82: #include "fastcgi.h"
	行   84: #include <php_config.h>
	行   85: #include "fpm.h"
	行   86: #include "fpm_main_arginfo.h"
	行   87: #include "fpm_request.h"
	行   88: #include "fpm_status.h"
	行   89: #include "fpm_signals.h"
	行   90: #include "fpm_stdio.h"
	行   91: #include "fpm_conf.h"
	行   92: #include "fpm_php.h"
	行   93: #include "fpm_log.h"
	行   94: #include "zlog.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_php.c （匹配14次）
	行   3: #include "fpm_config.h"
	行   5: #include <stdlib.h>
	行   6: #include <string.h>
	行   7: #include <stdio.h>
	行   9: #include "php.h"
	行  10: #include "php_main.h"
	行  11: #include "php_ini.h"
	行  12: #include "ext/standard/dl.h"
	行  14: #include "fastcgi.h"
	行  16: #include "fpm.h"
	行  17: #include "fpm_php.h"
	行  18: #include "fpm_cleanup.h"
	行  19: #include "fpm_worker_pool.h"
	行  20: #include "zlog.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_php.h （匹配4次）
	行  6: #include <TSRM.h>
	行  8: #include "php.h"
	行  9: #include "build-defs.h" /* for PHP_ defines */
	行 10: #include "fpm/fpm_conf.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_php_trace.c （匹配17次）
	行   3: #include "fpm_config.h"
	行   7: #include "php.h"
	行   8: #include "php_main.h"
	行  10: #include <stdio.h>
	行  11: #include <stddef.h>
	行  12: #include <inttypes.h>
	行  13: #include <unistd.h>
	行  14: #include <sys/time.h>
	行  15: #include <sys/types.h>
	行  16: #include <errno.h>
	行  18: #include "fpm_trace.h"
	行  19: #include "fpm_php_trace.h"
	行  20: #include "fpm_children.h"
	行  21: #include "fpm_worker_pool.h"
	行  22: #include "fpm_process_ctl.h"
	行  23: #include "fpm_scoreboard.h"
	行  25: #include "zlog.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_process_ctl.c （匹配18次）
	行   3: #include "fpm_config.h"
	行   5: #include <sys/types.h>
	行   6: #include <signal.h>
	行   7: #include <unistd.h>
	行   8: #include <stdlib.h>
	行  10: #include "fpm.h"
	行  11: #include "fpm_clock.h"
	行  12: #include "fpm_children.h"
	行  13: #include "fpm_signals.h"
	行  14: #include "fpm_events.h"
	行  15: #include "fpm_process_ctl.h"
	行  16: #include "fpm_cleanup.h"
	行  17: #include "fpm_request.h"
	行  18: #include "fpm_worker_pool.h"
	行  19: #include "fpm_scoreboard.h"
	行  20: #include "fpm_sockets.h"
	行  21: #include "fpm_stdio.h"
	行  22: #include "zlog.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_process_ctl.h （匹配1次）
	行  6: #include "fpm_events.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_request.c （匹配16次）
	行   3: #include <sys/times.h>
	行   6: #include "fpm_config.h"
	行   8: #include "fpm.h"
	行   9: #include "fpm_php.h"
	行  10: #include "fpm_str.h"
	行  11: #include "fpm_clock.h"
	行  12: #include "fpm_conf.h"
	行  13: #include "fpm_trace.h"
	行  14: #include "fpm_php_trace.h"
	行  15: #include "fpm_process_ctl.h"
	行  16: #include "fpm_children.h"
	行  17: #include "fpm_scoreboard.h"
	行  18: #include "fpm_status.h"
	行  19: #include "fpm_request.h"
	行  20: #include "fpm_log.h"
	行  22: #include "zlog.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_scoreboard.c （匹配12次）
	行   3: #include "php.h"
	行   4: #include "SAPI.h"
	行   5: #include <stdio.h>
	行   6: #include <time.h>
	行   8: #include "fpm_config.h"
	行   9: #include "fpm_children.h"
	行  10: #include "fpm_scoreboard.h"
	行  11: #include "fpm_shm.h"
	行  12: #include "fpm_sockets.h"
	行  13: #include "fpm_worker_pool.h"
	行  14: #include "fpm_clock.h"
	行  15: #include "zlog.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_scoreboard.h （匹配5次）
	行   6: #include <sys/time.h>
	行   8: #include <sys/times.h>
	行  11: #include "fpm_request.h"
	行  12: #include "fpm_worker_pool.h"
	行  13: #include "fpm_atomic.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_shm.c （匹配5次）
	行  3: #include <sys/mman.h>
	行  4: #include <errno.h>
	行  5: #include <string.h>
	行  7: #include "fpm_shm.h"
	行  8: #include "zlog.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_signals.c （匹配15次）
	行   3: #include "fpm_config.h"
	行   5: #include <signal.h>
	行   6: #include <stdio.h>
	行   7: #include <sys/types.h>
	行   8: #include <sys/socket.h>
	行   9: #include <stdlib.h>
	行  10: #include <string.h>
	行  11: #include <fcntl.h>
	行  12: #include <unistd.h>
	行  13: #include <errno.h>
	行  15: #include "fpm.h"
	行  16: #include "fpm_signals.h"
	行  17: #include "fpm_sockets.h"
	行  18: #include "fpm_php.h"
	行  19: #include "zlog.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_signals.h （匹配1次）
	行  6: #include <signal.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_sockets.c （匹配25次）
	行   3: #include "fpm_config.h"
	行   6: #include <alloca.h>
	行   8: #include <sys/types.h>
	行   9: #include <sys/stat.h> /* for chmod(2) */
	行  10: #include <sys/socket.h>
	行  11: #include <netinet/in.h>
	行  12: #include <arpa/inet.h>
	行  13: #include <sys/un.h>
	行  14: #include <netdb.h>
	行  15: #include <stdio.h>
	行  16: #include <stdlib.h>
	行  17: #include <string.h>
	行  18: #include <errno.h>
	行  19: #include <unistd.h>
	行  21: #include "zlog.h"
	行  22: #include "fpm_arrays.h"
	行  23: #include "fpm_sockets.h"
	行  24: #include "fpm_worker_pool.h"
	行  25: #include "fpm_unix.h"
	行  26: #include "fpm_str.h"
	行  27: #include "fpm_env.h"
	行  28: #include "fpm_cleanup.h"
	行  29: #include "fpm_scoreboard.h"
	行 531: #include <netinet/tcp.h>
	行 574: #include <netinet/tcp.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_sockets.h （匹配7次）
	行  6: #include <sys/types.h>
	行  7: #include <sys/socket.h>
	行  9: #include <sys/sysctl.h>
	行 11: #include <sys/un.h>
	行 12: #include <unistd.h>
	行 13: #include <fcntl.h>
	行 15: #include "fpm_worker_pool.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_status.c （匹配13次）
	行   3: #include "php.h"
	行   4: #include "zend_long.h"
	行   5: #include "SAPI.h"
	行   6: #include <stdio.h>
	行   8: #include "fpm_config.h"
	行   9: #include "fpm_scoreboard.h"
	行  10: #include "fpm_status.h"
	行  11: #include "fpm_clock.h"
	行  12: #include "zlog.h"
	行  13: #include "fpm_atomic.h"
	行  14: #include "fpm_conf.h"
	行  15: #include "fpm_php.h"
	行  16: #include <ext/standard/html.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_status.h （匹配2次）
	行  5: #include "fpm_worker_pool.h"
	行  6: #include "fpm_shm.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_stdio.c （匹配16次）
	行   3: #include "fpm_config.h"
	行   5: #include <sys/types.h>
	行   6: #include <sys/stat.h>
	行   7: #include <string.h>
	行   8: #include <fcntl.h>
	行   9: #include <unistd.h>
	行  10: #include <errno.h>
	行  12: #include "php_syslog.h"
	行  13: #include "php_network.h"
	行  15: #include "fpm.h"
	行  16: #include "fpm_children.h"
	行  17: #include "fpm_cleanup.h"
	行  18: #include "fpm_events.h"
	行  19: #include "fpm_sockets.h"
	行  20: #include "fpm_stdio.h"
	行  21: #include "zlog.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_stdio.h （匹配1次）
	行  6: #include "fpm_worker_pool.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_systemd.c （匹配9次）
	行   1: #include "fpm_config.h"
	行   3: #include <sys/types.h>
	行   4: #include <systemd/sd-daemon.h>
	行   6: #include "fpm.h"
	行   7: #include "fpm_clock.h"
	行   8: #include "fpm_worker_pool.h"
	行   9: #include "fpm_scoreboard.h"
	行  10: #include "zlog.h"
	行  11: #include "fpm_systemd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_systemd.h （匹配1次）
	行  4: #include "fpm_events.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_trace.c （匹配3次）
	行  3: #include "fpm_config.h"
	行  5: #include <sys/types.h>
	行  7: #include "fpm_trace.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_trace.h （匹配1次）
	行  6: #include <unistd.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_trace_mach.c （匹配8次）
	行  3: #include "fpm_config.h"
	行  5: #include <mach/mach.h>
	行  6: #include <mach/mach_vm.h>
	行  8: #include <unistd.h>
	行 10: #include "fpm_trace.h"
	行 11: #include "fpm_process_ctl.h"
	行 12: #include "fpm_unix.h"
	行 13: #include "zlog.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_trace_pread.c （匹配8次）
	行  8: #include "fpm_config.h"
	行 10: #include <unistd.h>
	行 12: #include <fcntl.h>
	行 13: #include <stdio.h>
	行 14: #include <inttypes.h>
	行 16: #include "fpm_trace.h"
	行 17: #include "fpm_process_ctl.h"
	行 18: #include "zlog.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_trace_ptrace.c （匹配7次）
	行  3: #include "fpm_config.h"
	行  5: #include <sys/wait.h>
	行  6: #include <sys/ptrace.h>
	行  7: #include <unistd.h>
	行  8: #include <errno.h>
	行 22: #include "fpm_trace.h"
	行 23: #include "zlog.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_unix.c （匹配23次）
	行   3: #include "fpm_config.h"
	行   5: #include <string.h>
	行   6: #include <sys/time.h>
	行   7: #include <sys/resource.h>
	行   8: #include <stdlib.h>
	行   9: #include <unistd.h>
	行  10: #include <sys/types.h>
	行  11: #include <pwd.h>
	行  12: #include <grp.h>
	行  15: #include <sys/prctl.h>
	行  19: #include <sys/procctl.h>
	行  23: #include <priv.h>
	行  27: #include <sys/apparmor.h>
	行  31: #include <sys/acl.h>
	行  35: #include <selinux/selinux.h>
	行  38: #include "fpm.h"
	行  39: #include "fpm_conf.h"
	行  40: #include "fpm_cleanup.h"
	行  41: #include "fpm_clock.h"
	行  42: #include "fpm_stdio.h"
	行  43: #include "fpm_unix.h"
	行  44: #include "fpm_signals.h"
	行  45: #include "zlog.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_unix.h （匹配1次）
	行  6: #include "fpm_worker_pool.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_worker_pool.c （匹配12次）
	行  3: #include "fpm_config.h"
	行  5: #include <string.h>
	行  6: #include <stdlib.h>
	行  7: #include <unistd.h>
	行  9: #include "fpm.h"
	行 10: #include "fpm_worker_pool.h"
	行 11: #include "fpm_cleanup.h"
	行 12: #include "fpm_children.h"
	行 13: #include "fpm_shm.h"
	行 14: #include "fpm_scoreboard.h"
	行 15: #include "fpm_conf.h"
	行 16: #include "fpm_unix.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\fpm_worker_pool.h （匹配2次）
	行  6: #include "fpm_conf.h"
	行  7: #include "fpm_shm.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\zlog.c （匹配12次）
	行   3: #include "fpm_config.h"
	行   5: #include <stdio.h>
	行   6: #include <unistd.h>
	行   7: #include <time.h>
	行   8: #include <string.h>
	行   9: #include <stdarg.h>
	行  10: #include <sys/time.h>
	行  11: #include <errno.h>
	行  13: #include "php_syslog.h"
	行  15: #include "zlog.h"
	行  16: #include "fpm.h"
	行  17: #include "zend_portability.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fpm\\fpm\\zlog.h （匹配2次）
	行   6: #include <stdarg.h>
	行   7: #include <sys/types.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fuzzer\\fuzzer-execute-common.h （匹配4次）
	行  17: #include <main/php.h>
	行  23: #include "fuzzer.h"
	行  24: #include "fuzzer-sapi.h"
	行  25: #include "zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fuzzer\\fuzzer-execute.c （匹配1次）
	行 17: #include "fuzzer-execute-common.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fuzzer\\fuzzer-exif.c （匹配12次）
	行 17: #include "fuzzer.h"
	行 19: #include "Zend/zend.h"
	行 20: #include "main/php_config.h"
	行 21: #include "main/php_main.h"
	行 22: #include "ext/standard/php_var.h"
	行 24: #include <stdio.h>
	行 25: #include <stdint.h>
	行 26: #include <stdlib.h>
	行 27: #include <sys/types.h>
	行 28: #include <sys/stat.h>
	行 29: #include <fcntl.h>
	行 31: #include "fuzzer-sapi.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fuzzer\\fuzzer-function-jit.c （匹配1次）
	行 17: #include "fuzzer-execute-common.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fuzzer\\fuzzer-json.c （匹配9次）
	行 20: #include "fuzzer.h"
	行 22: #include "Zend/zend.h"
	行 23: #include "main/php_config.h"
	行 24: #include "main/php_main.h"
	行 26: #include <stdio.h>
	行 27: #include <stdint.h>
	行 28: #include <stdlib.h>
	行 30: #include "fuzzer-sapi.h"
	行 31: #include "ext/json/php_json_parser.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fuzzer\\fuzzer-mbregex.c （匹配9次）
	行 18: #include "fuzzer.h"
	行 20: #include "Zend/zend.h"
	行 21: #include "main/php_config.h"
	行 22: #include "main/php_main.h"
	行 23: #include "oniguruma.h"
	行 25: #include <stdio.h>
	行 26: #include <stdint.h>
	行 27: #include <stdlib.h>
	行 29: #include "fuzzer-sapi.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fuzzer\\fuzzer-mbstring.c （匹配4次）
	行 18: #include "zend.h"
	行 19: #include "fuzzer.h"
	行 20: #include "fuzzer-sapi.h"
	行 21: #include "ext/mbstring/mbstring.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fuzzer\\fuzzer-parser.c （匹配8次）
	行 18: #include <main/php.h>
	行 19: #include <main/php_main.h>
	行 20: #include <main/SAPI.h>
	行 21: #include <ext/standard/info.h>
	行 22: #include <ext/standard/php_var.h>
	行 23: #include <main/php_variables.h>
	行 25: #include "fuzzer.h"
	行 26: #include "fuzzer-sapi.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fuzzer\\fuzzer-sapi.c （匹配9次）
	行  18: #include <main/php.h>
	行  19: #include <main/php_main.h>
	行  20: #include <main/SAPI.h>
	行  21: #include <ext/standard/info.h>
	行  22: #include <ext/standard/php_var.h>
	行  23: #include <main/php_variables.h>
	行  24: #include <zend_exceptions.h>
	行  30: #include "fuzzer.h"
	行  31: #include "fuzzer-sapi.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fuzzer\\fuzzer-tracing-jit.c （匹配1次）
	行 17: #include "fuzzer-execute-common.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fuzzer\\fuzzer-unserialize.c （匹配9次）
	行 18: #include "fuzzer.h"
	行 20: #include "Zend/zend.h"
	行 21: #include "main/php_config.h"
	行 22: #include "main/php_main.h"
	行 24: #include <stdio.h>
	行 25: #include <stdint.h>
	行 26: #include <stdlib.h>
	行 28: #include "fuzzer-sapi.h"
	行 30: #include "ext/standard/php_var.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fuzzer\\fuzzer-unserializehash.c （匹配9次）
	行 16: #include "fuzzer.h"
	行 18: #include "Zend/zend.h"
	行 19: #include "main/php_config.h"
	行 20: #include "main/php_main.h"
	行 22: #include <stdio.h>
	行 23: #include <stdint.h>
	行 24: #include <stdlib.h>
	行 26: #include "fuzzer-sapi.h"
	行 28: #include "ext/standard/php_var.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\fuzzer\\fuzzer.h （匹配1次）
	行 17: #include "php_version.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\litespeed\\lsapidef.h （匹配1次）
	行  53: #include <inttypes.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\litespeed\\lsapilib.c （匹配35次）
	行   50: #include <ctype.h>
	行   51: #include <dlfcn.h>
	行   52: #include <errno.h>
	行   53: #include <fcntl.h>
	行   54: #include <limits.h>
	行   55: #include <sys/stat.h>
	行   56: #include <sched.h>
	行   57: #include <signal.h>
	行   58: #include <stdlib.h>
	行   59: #include <stdio.h>
	行   60: #include <stdarg.h>
	行   61: #include <string.h>
	行   62: #include <sys/mman.h>
	行   63: #include <sys/resource.h>
	行   64: #include <sys/socket.h>
	行   65: #include <sys/time.h>
	行   66: #include <sys/uio.h>
	行   67: #include <sys/wait.h>
	行   68: #include <grp.h>
	行   69: #include <pwd.h>
	行   70: #include <time.h>
	行   71: #include <unistd.h>
	行   72: #include <arpa/inet.h>
	行   73: #include <netdb.h>
	行   74: #include <netinet/in.h>
	行   75: #include <netinet/tcp.h>
	行   76: #include <sys/un.h>
	行   78: #include "lsapilib.h"
	行   81: #include <sys/prctl.h>
	行   86: #include <sys/sysctl.h>
	行   89: #include <inttypes.h>
	行   94: #include <Zend/zend_portability.h>
	行 1995: #include <sys/sendfile.h>
	行 2020: #include <sys/sendfile.h>
	行 3811: #include <crt_externs.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\litespeed\\lsapilib.h （匹配4次）
	行  57: #include "lsapidef.h"
	行  59: #include <stddef.h>
	行  60: #include <sys/time.h>
	行  61: #include <sys/types.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\litespeed\\lsapi_main.c （匹配25次）
	行   17: #include "php.h"
	行   18: #include "SAPI.h"
	行   19: #include "php_main.h"
	行   20: #include "php_ini.h"
	行   21: #include "php_variables.h"
	行   22: #include "zend_highlight.h"
	行   23: #include "zend.h"
	行   24: #include "ext/standard/basic_functions.h"
	行   25: #include "ext/standard/info.h"
	行   26: #include "ext/standard/head.h"
	行   27: #include "lsapilib.h"
	行   28: #include "lsapi_main_arginfo.h"
	行   30: #include <stdio.h>
	行   31: #include <stdlib.h>
	行   34: #include <unistd.h>
	行   37: #include <sys/wait.h>
	行   38: #include <sys/stat.h>
	行   42: #include <sys/types.h>
	行   46: #include <signal.h>
	行   47: #include <sys/socket.h>
	行   48: #include <arpa/inet.h>
	行   49: #include <netinet/in.h>
	行   50: #include <sys/time.h>
	行   53: #include "lscriu.c"
	行 1447: #include <fcntl.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\litespeed\\lscriu.c （匹配26次）
	行  49: #include "lsapilib.h"
	行  51: #include <stdio.h>
	行  52: #include <stdlib.h>
	行  55: #include <unistd.h>
	行  58: #include <sys/wait.h>
	行  61: #include <sys/stat.h>
	行  65: #include <sys/types.h>
	行  69: #include <sys/types.h>
	行  70: #include <sys/socket.h>
	行  71: #include <sys/un.h>
	行  72: #include <arpa/inet.h>
	行  73: #include <netinet/in.h>
	行  74: #include <semaphore.h>
	行  75: #include <sys/mman.h>
	行  76: #include <fcntl.h>
	行  77: #include <dlfcn.h>
	行  78: #include <stdlib.h>
	行  79: #include <errno.h>
	行  80: #include <string.h>
	行  81: #include <stdarg.h>
	行  83: #include <signal.h>
	行  84: #include <time.h>
	行  85: #include <sys/timeb.h>
	行  86: #include <unistd.h>
	行  87: #include "lscriu.h"
	行  89: #include <Zend/zend_portability.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg.c （匹配15次）
	行   19: #include "phpdbg.h"
	行   20: #include "phpdbg_prompt.h"
	行   21: #include "phpdbg_bp.h"
	行   22: #include "phpdbg_break.h"
	行   23: #include "phpdbg_list.h"
	行   24: #include "phpdbg_utils.h"
	行   25: #include "phpdbg_set.h"
	行   26: #include "phpdbg_io.h"
	行   27: #include "zend_alloc.h"
	行   28: #include "phpdbg_print.h"
	行   29: #include "phpdbg_help.h"
	行   30: #include "phpdbg_arginfo.h"
	行   31: #include "zend_vm.h"
	行   32: #include "php_ini_builder.h"
	行   34: #include "ext/standard/basic_functions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg.h （匹配25次）
	行  30: #include <stdint.h>
	行  31: #include <stddef.h>
	行  32: #include "php.h"
	行  33: #include "php_globals.h"
	行  34: #include "php_variables.h"
	行  35: #include "php_getopt.h"
	行  36: #include "zend_builtin_functions.h"
	行  37: #include "zend_extensions.h"
	行  38: #include "zend_modules.h"
	行  39: #include "zend_globals.h"
	行  40: #include "zend_ini_scanner.h"
	行  41: #include "zend_stream.h"
	行  42: #include "zend_signal.h"
	行  48: #include "SAPI.h"
	行  49: #include <fcntl.h>
	行  50: #include <sys/types.h>
	行  64: #include "php_main.h"
	行 105: #include "phpdbg_sigsafe.h"
	行 106: #include "phpdbg_out.h"
	行 107: #include "phpdbg_lexer.h"
	行 108: #include "phpdbg_cmd.h"
	行 109: #include "phpdbg_utils.h"
	行 110: #include "phpdbg_btree.h"
	行 111: #include "phpdbg_watch.h"
	行 112: #include "phpdbg_bp.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_bp.c （匹配7次）
	行   19: #include "zend.h"
	行   20: #include "zend_hash.h"
	行   21: #include "phpdbg.h"
	行   22: #include "phpdbg_bp.h"
	行   23: #include "phpdbg_utils.h"
	行   24: #include "zend_globals.h"
	行   25: #include "ext/standard/php_string.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_break.c （匹配6次）
	行 19: #include "phpdbg.h"
	行 20: #include "phpdbg_print.h"
	行 21: #include "phpdbg_utils.h"
	行 22: #include "phpdbg_break.h"
	行 23: #include "phpdbg_bp.h"
	行 24: #include "phpdbg_prompt.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_break.h （匹配2次）
	行 22: #include "TSRM.h"
	行 23: #include "phpdbg_cmd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_btree.c （匹配2次）
	行  19: #include "phpdbg_btree.h"
	行  20: #include "phpdbg.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_btree.h （匹配1次）
	行 22: #include "zend.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_cmd.c （匹配6次）
	行  19: #include "phpdbg.h"
	行  20: #include "phpdbg_cmd.h"
	行  21: #include "phpdbg_utils.h"
	行  22: #include "phpdbg_set.h"
	行  23: #include "phpdbg_prompt.h"
	行  24: #include "phpdbg_io.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_cmd.h （匹配2次）
	行  22: #include "TSRM.h"
	行  23: #include "zend_generators.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_frame.c （匹配6次）
	行  19: #include "zend.h"
	行  20: #include "phpdbg.h"
	行  21: #include "phpdbg_utils.h"
	行  22: #include "phpdbg_frame.h"
	行  23: #include "phpdbg_list.h"
	行  24: #include "zend_smart_str.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_frame.h （匹配1次）
	行 22: #include "TSRM.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_help.c （匹配4次）
	行   20: #include "phpdbg.h"
	行   21: #include "phpdbg_help.h"
	行   22: #include "phpdbg_prompt.h"
	行   23: #include "zend.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_help.h （匹配3次）
	行 22: #include "TSRM.h"
	行 23: #include "phpdbg.h"
	行 24: #include "phpdbg_cmd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_info.c （匹配6次）
	行  19: #include "php.h"
	行  20: #include "phpdbg.h"
	行  21: #include "phpdbg_utils.h"
	行  22: #include "phpdbg_info.h"
	行  23: #include "phpdbg_bp.h"
	行  24: #include "phpdbg_prompt.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_info.h （匹配1次）
	行 22: #include "phpdbg_cmd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_io.c （匹配2次）
	行  18: #include "config.h"
	行  21: #include "phpdbg_io.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_io.h （匹配1次）
	行 20: #include "phpdbg.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_lexer.h （匹配1次）
	行 22: #include "phpdbg_cmd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_list.c （匹配10次）
	行  19: #include <stdio.h>
	行  20: #include <string.h>
	行  21: #include <sys/stat.h>
	行  26: #include <fcntl.h>
	行  27: #include "phpdbg.h"
	行  28: #include "phpdbg_list.h"
	行  29: #include "phpdbg_utils.h"
	行  30: #include "phpdbg_prompt.h"
	行  31: #include "php_streams.h"
	行  32: #include "zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_list.h （匹配2次）
	行 22: #include "TSRM.h"
	行 23: #include "phpdbg_cmd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_out.c （匹配6次）
	行  19: #include "zend.h"
	行  20: #include "php.h"
	行  21: #include "spprintf.h"
	行  22: #include "phpdbg.h"
	行  23: #include "phpdbg_io.h"
	行  24: #include "ext/standard/html.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_print.c （匹配5次）
	行  19: #include "phpdbg.h"
	行  20: #include "phpdbg_print.h"
	行  21: #include "phpdbg_utils.h"
	行  22: #include "phpdbg_prompt.h"
	行  24: #include "Optimizer/zend_dump.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_print.h （匹配1次）
	行 22: #include "phpdbg_cmd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_prompt.c （匹配26次）
	行   19: #include <stdio.h>
	行   20: #include <string.h>
	行   21: #include "zend.h"
	行   22: #include "zend_compile.h"
	行   23: #include "zend_exceptions.h"
	行   24: #include "zend_vm.h"
	行   25: #include "zend_generators.h"
	行   26: #include "zend_interfaces.h"
	行   27: #include "zend_smart_str.h"
	行   28: #include "phpdbg.h"
	行   29: #include "phpdbg_io.h"
	行   31: #include "phpdbg_help.h"
	行   32: #include "phpdbg_print.h"
	行   33: #include "phpdbg_info.h"
	行   34: #include "phpdbg_break.h"
	行   35: #include "phpdbg_list.h"
	行   36: #include "phpdbg_utils.h"
	行   37: #include "phpdbg_prompt.h"
	行   38: #include "phpdbg_cmd.h"
	行   39: #include "phpdbg_set.h"
	行   40: #include "phpdbg_frame.h"
	行   41: #include "phpdbg_lexer.h"
	行   42: #include "phpdbg_parser.h"
	行   53: #include "win32/param.h"
	行   54: #include "win32/winutil.h"
	行   57: #include <sys/param.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_set.c （匹配6次）
	行  19: #include "phpdbg.h"
	行  20: #include "phpdbg_cmd.h"
	行  21: #include "phpdbg_set.h"
	行  22: #include "phpdbg_utils.h"
	行  23: #include "phpdbg_bp.h"
	行  24: #include "phpdbg_prompt.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_set.h （匹配1次）
	行 22: #include "phpdbg_cmd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_sigsafe.c （匹配2次）
	行  1: #include "phpdbg_sigsafe.h"
	行  2: #include "phpdbg.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_sigsafe.h （匹配2次）
	行  6: #include "zend.h"
	行 15: #include "phpdbg.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_utils.c （匹配5次）
	行  19: #include "zend.h"
	行  21: #include "php.h"
	行  22: #include "phpdbg.h"
	行  23: #include "phpdbg_utils.h"
	行  24: #include "ext/standard/php_string.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_watch.c （匹配6次）
	行  103: #include "zend.h"
	行  104: #include "phpdbg.h"
	行  105: #include "phpdbg_btree.h"
	行  106: #include "phpdbg_watch.h"
	行  107: #include "phpdbg_utils.h"
	行  108: #include "phpdbg_prompt.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_watch.h （匹配1次）
	行  22: #include "phpdbg_cmd.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_win.c （匹配2次）
	行 19: #include "zend.h"
	行 20: #include "phpdbg.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\sapi\\phpdbg\\phpdbg_win.h （匹配3次）
	行 22: #include "winbase.h"
	行 23: #include "windows.h"
	行 24: #include "excpt.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\TSRM\\TSRM.c （匹配3次）
	行  13: #include "TSRM.h"
	行  17: #include <stdio.h>
	行  18: #include <stdarg.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\TSRM\\TSRM.h （匹配3次）
	行  23: #include <stdint.h>
	行  24: #include <stdbool.h>
	行  71: #include <signal.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\TSRM\\tsrm_win32.c （匹配13次）
	行  17: #include <stdio.h>
	行  18: #include <fcntl.h>
	行  19: #include <io.h>
	行  20: #include <process.h>
	行  21: #include <time.h>
	行  22: #include <errno.h>
	行  25: #include "SAPI.h"
	行  26: #include "TSRM.h"
	行  29: #include <Sddl.h>
	行  30: #include "tsrm_win32.h"
	行  31: #include "zend_virtual_cwd.h"
	行  32: #include "win32/ioutil.h"
	行  33: #include "win32/winutil.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\TSRM\\tsrm_win32.h （匹配4次）
	行  20: #include "TSRM.h"
	行  21: #include <windows.h>
	行  22: #include <sys/utime.h>
	行  23: #include "win32/ipc.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\build\\deplister.c （匹配3次）
	行 20: #include <windows.h>
	行 21: #include <stdio.h>
	行 22: #include <imagehlp.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\codepage.c （匹配5次）
	行  17: #include <assert.h>
	行  19: #include "php.h"
	行  20: #include "SAPI.h"
	行  27: #include "win32/console.h"
	行  36: #include "cp_enc_map.c"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\console.c （匹配3次）
	行  17: #include "php.h"
	行  18: #include "SAPI.h"
	行  19: #include "win32/console.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\console.h （匹配3次）
	行 28: #include "php.h"
	行 29: #include "php_streams.h"
	行 30: #include <windows.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\cp_enc_map_gen.c （匹配2次）
	行   1: #include <stdio.h>
	行   3: #include <windows.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\dllmain.c （匹配5次）
	行 17: #include <config.w32.h>
	行 19: #include <win32/time.h>
	行 20: #include <win32/ioutil.h>
	行 21: #include <php.h>
	行 24: #include <libxml/threads.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\fnmatch.c （匹配4次）
	行  48: #include <ctype.h>
	行  49: #include <string.h>
	行  50: #include <stdio.h>
	行  52: #include "fnmatch.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\fnmatch.h （匹配1次）
	行 40: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\ftok.c （匹配5次）
	行 17: #include "php.h"
	行 18: #include "ipc.h"
	行 20: #include <windows.h>
	行 21: #include <sys/stat.h>
	行 23: #include "ioutil.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\getrusage.c （匹配3次）
	行 17: #include <php.h>
	行 18: #include <psapi.h>
	行 19: #include "getrusage.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\glob.c （匹配12次）
	行  74: #include "php.h"
	行  75: #include <sys/stat.h>
	行  77: #include <ctype.h>
	行  79: #include <sys/param.h>
	行  80: #include <dirent.h>
	行  81: #include <pwd.h>
	行  82: #include <unistd.h>
	行  84: #include <errno.h>
	行  85: #include "glob.h"
	行  86: #include <stdio.h>
	行  87: #include <stdlib.h>
	行  88: #include <string.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\glob.h （匹配1次）
	行  49: #include "Zend/zend_stream.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\globals.c （匹配3次）
	行 17: #include "php.h"
	行 18: #include "php_win32_globals.h"
	行 19: #include "syslog.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\inet.c （匹配1次）
	行 17: #include "inet.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\inet.h （匹配2次）
	行 20: #include <php.h>
	行 21: #include <Winsock2.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\ioutil.c （匹配21次）
	行   41: #include <assert.h>
	行   42: #include <stdlib.h>
	行   43: #include <direct.h>
	行   44: #include <errno.h>
	行   45: #include <fcntl.h>
	行   46: #include <io.h>
	行   47: #include <limits.h>
	行   48: #include <sys/stat.h>
	行   49: #include <sys/utime.h>
	行   50: #include <stdio.h>
	行   52: #include "php.h"
	行   53: #include "SAPI.h"
	行   54: #include "win32/winutil.h"
	行   55: #include "win32/time.h"
	行   56: #include "win32/ioutil.h"
	行   57: #include "win32/codepage.h"
	行   58: #include "main/streams/php_stream_plain_wrapper.h"
	行   60: #include <pathcch.h>
	行   61: #include <winioctl.h>
	行   62: #include <winnt.h>
	行   67: #include <winnls.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\ioutil.h （匹配9次）
	行  44: #include <fcntl.h>
	行  45: #include <sys/types.h>
	行  46: #include <sys/stat.h>
	行  47: #include <io.h>
	行  48: #include <stdio.h>
	行  49: #include <stdlib.h>
	行  51: #include "win32/winutil.h"
	行  52: #include "win32/codepage.h"
	行 685: #include <sys/stat.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\nice.c （匹配2次）
	行 17: #include <php.h>
	行 18: #include "nice.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\param.h （匹配1次）
	行 14: #include "win32/ioutil.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\php_win32_globals.h （匹配1次）
	行 22: #include "win32/sendmail.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\readdir.c （匹配6次）
	行   1: #include "php.h"
	行   3: #include <malloc.h>
	行   4: #include <string.h>
	行   5: #include <errno.h>
	行   7: #include "readdir.h"
	行   8: #include "win32/ioutil.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\readdir.h （匹配2次）
	行 14: #include <config.w32.h>
	行 16: #include "ioutil.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\registry.c （匹配3次）
	行  17: #include "php.h"
	行  18: #include "php_ini.h"
	行  19: #include "php_win32_globals.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\select.c （匹配2次）
	行  17: #include "php.h"
	行  18: #include "php_network.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\select.h （匹配1次）
	行 20: #include "php_network.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\sendmail.c （匹配16次）
	行   20: #include "php.h"				/*php specific */
	行   21: #include <stdio.h>
	行   22: #include <stdlib.h>
	行   23: #include <winsock2.h>
	行   24: #include "time.h"
	行   26: #include <string.h>
	行   27: #include <math.h>
	行   28: #include <malloc.h>
	行   29: #include <winbase.h>
	行   30: #include "sendmail.h"
	行   31: #include "php_ini.h"
	行   32: #include "inet.h"
	行   34: #include "php_win32_globals.h"
	行   36: #include "ext/pcre/php_pcre.h"
	行   37: #include "ext/standard/php_string.h"
	行   38: #include "ext/date/php_date.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\sendmail.h （匹配1次）
	行  3: #include <windows.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\signal.c （匹配3次）
	行  17: #include "php.h"
	行  18: #include "SAPI.h"
	行  20: #include "win32/console.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\signal.h （匹配2次）
	行  4: #include <signal.h>
	行  6: #include "win32/winutil.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\sockets.c （匹配3次）
	行 22: #include <stdio.h>
	行 23: #include <fcntl.h>
	行 25: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\syslog.h （匹配1次）
	行 20: #include <windows.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\time.c （匹配9次）
	行  15: #include <config.w32.h>
	行  17: #include "time.h"
	行  18: #include "unistd.h"
	行  19: #include "signal.h"
	行  20: #include <windows.h>
	行  21: #include <winbase.h>
	行  22: #include <mmsystem.h>
	行  23: #include <errno.h>
	行  24: #include "php_win32_globals.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\time.h （匹配2次）
	行 16: #include <time.h>
	行 17: #include "php.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\winutil.c （匹配5次）
	行  18: #include "php.h"
	行  19: #include "winutil.h"
	行  20: #include "codepage.h"
	行  21: #include <bcrypt.h>
	行  22: #include <lmcons.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\win32\\wsyslog.c （匹配8次）
	行  53: #include "php.h"				/*php specific */
	行  54: #include "syslog.h"
	行  55: #include <stdio.h>
	行  56: #include <fcntl.h>
	行  57: #include <process.h>
	行  59: #include "php_win32_globals.h"
	行  60: #include "wsyslog.h"
	行  61: #include "codepage.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\block_pass.c （匹配9次）
	行   22: #include "Optimizer/zend_optimizer.h"
	行   23: #include "Optimizer/zend_optimizer_internal.h"
	行   24: #include "zend_API.h"
	行   25: #include "zend_constants.h"
	行   26: #include "zend_execute.h"
	行   27: #include "zend_vm.h"
	行   28: #include "zend_bitset.h"
	行   29: #include "zend_cfg.h"
	行   30: #include "zend_dump.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\compact_literals.c （匹配7次）
	行  24: #include "Optimizer/zend_optimizer.h"
	行  25: #include "Optimizer/zend_optimizer_internal.h"
	行  26: #include "zend_API.h"
	行  27: #include "zend_constants.h"
	行  28: #include "zend_execute.h"
	行  29: #include "zend_vm.h"
	行  30: #include "zend_extensions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\compact_vars.c （匹配2次）
	行  19: #include "Optimizer/zend_optimizer_internal.h"
	行  20: #include "zend_bitset.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\dce.c （匹配6次）
	行  20: #include "Optimizer/zend_optimizer_internal.h"
	行  21: #include "Optimizer/zend_inference.h"
	行  22: #include "Optimizer/zend_ssa.h"
	行  23: #include "Optimizer/zend_func_info.h"
	行  24: #include "Optimizer/zend_call_graph.h"
	行  25: #include "zend_bitset.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\dfa_pass.c （匹配13次）
	行   19: #include "Optimizer/zend_optimizer.h"
	行   20: #include "Optimizer/zend_optimizer_internal.h"
	行   21: #include "zend_API.h"
	行   22: #include "zend_constants.h"
	行   23: #include "zend_execute.h"
	行   24: #include "zend_vm.h"
	行   25: #include "zend_bitset.h"
	行   26: #include "zend_cfg.h"
	行   27: #include "zend_ssa.h"
	行   28: #include "zend_func_info.h"
	行   29: #include "zend_call_graph.h"
	行   30: #include "zend_inference.h"
	行   31: #include "zend_dump.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\escape_analysis.c （匹配7次）
	行  19: #include "Optimizer/zend_optimizer.h"
	行  20: #include "Optimizer/zend_optimizer_internal.h"
	行  21: #include "zend_bitset.h"
	行  22: #include "zend_cfg.h"
	行  23: #include "zend_ssa.h"
	行  24: #include "zend_inference.h"
	行  25: #include "zend_dump.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\nop_removal.c （匹配6次）
	行 26: #include "Optimizer/zend_optimizer.h"
	行 27: #include "Optimizer/zend_optimizer_internal.h"
	行 28: #include "zend_API.h"
	行 29: #include "zend_constants.h"
	行 30: #include "zend_execute.h"
	行 31: #include "zend_vm.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\optimize_func_calls.c （匹配6次）
	行  24: #include "Optimizer/zend_optimizer.h"
	行  25: #include "Optimizer/zend_optimizer_internal.h"
	行  26: #include "zend_API.h"
	行  27: #include "zend_constants.h"
	行  28: #include "zend_execute.h"
	行  29: #include "zend_vm.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\optimize_temp_vars_5.c （匹配7次）
	行  22: #include "Optimizer/zend_optimizer.h"
	行  23: #include "Optimizer/zend_optimizer_internal.h"
	行  24: #include "zend_API.h"
	行  25: #include "zend_constants.h"
	行  26: #include "zend_execute.h"
	行  27: #include "zend_vm.h"
	行  28: #include "zend_bitset.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\pass1.c （匹配6次）
	行  30: #include "Optimizer/zend_optimizer.h"
	行  31: #include "Optimizer/zend_optimizer_internal.h"
	行  32: #include "zend_API.h"
	行  33: #include "zend_constants.h"
	行  34: #include "zend_execute.h"
	行  35: #include "zend_vm.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\pass3.c （匹配6次）
	行  26: #include "Optimizer/zend_optimizer.h"
	行  27: #include "Optimizer/zend_optimizer_internal.h"
	行  28: #include "zend_API.h"
	行  29: #include "zend_constants.h"
	行  30: #include "zend_execute.h"
	行  31: #include "zend_vm.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\sccp.c （匹配9次）
	行   20: #include "zend_API.h"
	行   21: #include "zend_exceptions.h"
	行   22: #include "zend_ini.h"
	行   23: #include "zend_type_info.h"
	行   24: #include "Optimizer/zend_optimizer_internal.h"
	行   25: #include "Optimizer/zend_call_graph.h"
	行   26: #include "Optimizer/zend_inference.h"
	行   27: #include "Optimizer/scdf.h"
	行   28: #include "Optimizer/zend_dump.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\scdf.c （匹配2次）
	行  19: #include "Optimizer/zend_optimizer_internal.h"
	行  20: #include "Optimizer/scdf.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\scdf.h （匹配1次）
	行  22: #include "zend_bitset.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\ssa_integrity.c （匹配1次）
	行  19: #include "Optimizer/zend_optimizer_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\zend_call_graph.c （匹配9次）
	行  19: #include "zend_compile.h"
	行  20: #include "zend_extensions.h"
	行  21: #include "Optimizer/zend_optimizer.h"
	行  22: #include "zend_optimizer_internal.h"
	行  23: #include "zend_inference.h"
	行  24: #include "zend_call_graph.h"
	行  25: #include "zend_func_info.h"
	行  26: #include "zend_inference.h"
	行  27: #include "zend_call_graph.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\zend_call_graph.h （匹配3次）
	行 22: #include "zend_ssa.h"
	行 23: #include "zend_func_info.h"
	行 24: #include "zend_optimizer.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\zend_cfg.c （匹配7次）
	行  19: #include "zend_compile.h"
	行  20: #include "zend_cfg.h"
	行  21: #include "zend_func_info.h"
	行  22: #include "zend_worklist.h"
	行  23: #include "zend_optimizer.h"
	行  24: #include "zend_optimizer_internal.h"
	行  25: #include "zend_sort.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\zend_dfg.c （匹配2次）
	行  19: #include "zend_compile.h"
	行  20: #include "zend_dfg.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\zend_dfg.h （匹配2次）
	行 22: #include "zend_bitset.h"
	行 23: #include "zend_cfg.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\zend_dump.c （匹配7次）
	行   19: #include "zend_compile.h"
	行   20: #include "zend_cfg.h"
	行   21: #include "zend_ssa.h"
	行   22: #include "zend_inference.h"
	行   23: #include "zend_func_info.h"
	行   24: #include "zend_call_graph.h"
	行   25: #include "zend_dump.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\zend_dump.h （匹配2次）
	行 22: #include "zend_ssa.h"
	行 23: #include "zend_dfg.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\zend_func_info.c （匹配10次）
	行  20: #include "zend_compile.h"
	行  21: #include "zend_extensions.h"
	行  22: #include "zend_ssa.h"
	行  23: #include "zend_optimizer_internal.h"
	行  24: #include "zend_inference.h"
	行  25: #include "zend_call_graph.h"
	行  26: #include "zend_func_info.h"
	行  27: #include "zend_inference.h"
	行  29: #include "win32/ioutil.h"
	行  50: #include "zend_func_infos.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\zend_func_info.h （匹配1次）
	行 22: #include "zend_ssa.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\zend_inference.c （匹配8次）
	行   19: #include "zend_compile.h"
	行   20: #include "zend_generators.h"
	行   21: #include "zend_inference.h"
	行   22: #include "zend_func_info.h"
	行   23: #include "zend_call_graph.h"
	行   24: #include "zend_closures.h"
	行   25: #include "zend_worklist.h"
	行   26: #include "zend_optimizer_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\zend_inference.h （匹配4次）
	行  22: #include "zend_optimizer.h"
	行  23: #include "zend_ssa.h"
	行  24: #include "zend_bitset.h"
	行  27: #include "zend_type_info.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\zend_optimizer.c （匹配13次）
	行   22: #include "Optimizer/zend_optimizer.h"
	行   23: #include "Optimizer/zend_optimizer_internal.h"
	行   24: #include "zend_API.h"
	行   25: #include "zend_constants.h"
	行   26: #include "zend_execute.h"
	行   27: #include "zend_vm.h"
	行   28: #include "zend_cfg.h"
	行   29: #include "zend_func_info.h"
	行   30: #include "zend_call_graph.h"
	行   31: #include "zend_inference.h"
	行   32: #include "zend_dump.h"
	行   33: #include "php.h"
	行   34: #include "zend_observer.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\zend_optimizer.h （匹配2次）
	行  25: #include "zend.h"
	行  26: #include "zend_compile.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\zend_optimizer_internal.h （匹配2次）
	行  25: #include "zend_ssa.h"
	行  26: #include "zend_func_info.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\zend_ssa.c （匹配6次）
	行   20: #include "zend_compile.h"
	行   21: #include "zend_dfg.h"
	行   22: #include "zend_ssa.h"
	行   23: #include "zend_dump.h"
	行   24: #include "zend_inference.h"
	行   25: #include "Optimizer/zend_optimizer_internal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\zend_ssa.h （匹配2次）
	行  22: #include "zend_optimizer.h"
	行  23: #include "zend_cfg.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\Optimizer\\zend_worklist.h （匹配2次）
	行  22: #include "zend_arena.h"
	行  23: #include "zend_bitset.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend.c （匹配21次）
	行   20: #include "zend.h"
	行   21: #include "zend_extensions.h"
	行   22: #include "zend_modules.h"
	行   23: #include "zend_constants.h"
	行   24: #include "zend_list.h"
	行   25: #include "zend_API.h"
	行   26: #include "zend_exceptions.h"
	行   27: #include "zend_builtin_functions.h"
	行   28: #include "zend_ini.h"
	行   29: #include "zend_vm.h"
	行   30: #include "zend_dtrace.h"
	行   31: #include "zend_virtual_cwd.h"
	行   32: #include "zend_smart_str.h"
	行   33: #include "zend_smart_string.h"
	行   34: #include "zend_cpuinfo.h"
	行   35: #include "zend_attributes.h"
	行   36: #include "zend_observer.h"
	行   37: #include "zend_fibers.h"
	行   38: #include "zend_max_execution_timer.h"
	行   39: #include "Optimizer/zend_optimizer.h"
	行  831: #include <floatingpoint.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend.h （匹配18次）
	行  27: #include "zend_types.h"
	行  28: #include "zend_map_ptr.h"
	行  29: #include "zend_errors.h"
	行  30: #include "zend_alloc.h"
	行  31: #include "zend_llist.h"
	行  32: #include "zend_string.h"
	行  33: #include "zend_hash.h"
	行  34: #include "zend_ast.h"
	行  35: #include "zend_gc.h"
	行  36: #include "zend_variables.h"
	行  37: #include "zend_iterators.h"
	行  38: #include "zend_stream.h"
	行  39: #include "zend_smart_str_public.h"
	行  40: #include "zend_smart_string_public.h"
	行  41: #include "zend_signal.h"
	行  42: #include "zend_max_execution_timer.h"
	行 412: #include "zend_object_handlers.h"
	行 413: #include "zend_operators.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_alloc.c （匹配16次）
	行   54: #include "zend.h"
	行   55: #include "zend_alloc.h"
	行   56: #include "zend_globals.h"
	行   57: #include "zend_operators.h"
	行   58: #include "zend_multiply.h"
	行   59: #include "zend_bitset.h"
	行   60: #include "zend_mmap.h"
	行   61: #include <signal.h>
	行   73: #include <stdio.h>
	行   74: #include <stdlib.h>
	行   75: #include <string.h>
	行   77: #include <sys/types.h>
	行   78: #include <sys/stat.h>
	行   79: #include <limits.h>
	行   80: #include <fcntl.h>
	行   81: #include <errno.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_alloc.h （匹配4次）
	行  24: #include <stdio.h>
	行  26: #include "../TSRM/TSRM.h"
	行  27: #include "zend.h"
	行  80: #include "zend_alloc_sizes.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_API.c （匹配14次）
	行   22: #include "zend.h"
	行   23: #include "zend_execute.h"
	行   24: #include "zend_API.h"
	行   25: #include "zend_modules.h"
	行   26: #include "zend_extensions.h"
	行   27: #include "zend_constants.h"
	行   28: #include "zend_interfaces.h"
	行   29: #include "zend_exceptions.h"
	行   30: #include "zend_closures.h"
	行   31: #include "zend_inheritance.h"
	行   32: #include "zend_ini.h"
	行   33: #include "zend_enum.h"
	行   34: #include "zend_observer.h"
	行   36: #include <stdarg.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_API.h （匹配6次）
	行   25: #include "zend_modules.h"
	行   26: #include "zend_list.h"
	行   27: #include "zend_operators.h"
	行   28: #include "zend_variables.h"
	行   29: #include "zend_execute.h"
	行   30: #include "zend_type_info.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_arena.h （匹配1次）
	行  22: #include "zend.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_ast.c （匹配8次）
	行   20: #include "zend_ast.h"
	行   21: #include "zend_API.h"
	行   22: #include "zend_operators.h"
	行   23: #include "zend_language_parser.h"
	行   24: #include "zend_smart_str.h"
	行   25: #include "zend_exceptions.h"
	行   26: #include "zend_constants.h"
	行   27: #include "zend_enum.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_ast.h （匹配1次）
	行  24: #include "zend.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_atomic.c （匹配1次）
	行 15: #include "zend_atomic.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_atomic.h （匹配2次）
	行  18: #include "zend_portability.h"
	行  20: #include <stdbool.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_attributes.c （匹配6次）
	行  20: #include "zend.h"
	行  21: #include "zend_API.h"
	行  22: #include "zend_attributes.h"
	行  23: #include "zend_attributes_arginfo.h"
	行  24: #include "zend_exceptions.h"
	行  25: #include "zend_smart_str.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_builtin_functions.c （匹配14次）
	行   20: #include "zend.h"
	行   21: #include "zend_API.h"
	行   22: #include "zend_attributes.h"
	行   23: #include "zend_gc.h"
	行   24: #include "zend_builtin_functions.h"
	行   25: #include "zend_constants.h"
	行   26: #include "zend_ini.h"
	行   27: #include "zend_interfaces.h"
	行   28: #include "zend_exceptions.h"
	行   29: #include "zend_extensions.h"
	行   30: #include "zend_closures.h"
	行   31: #include "zend_generators.h"
	行   32: #include "zend_builtin_functions_arginfo.h"
	行   33: #include "zend_smart_str.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_closures.c （匹配9次）
	行  21: #include "zend.h"
	行  22: #include "zend_API.h"
	行  23: #include "zend_closures.h"
	行  24: #include "zend_exceptions.h"
	行  25: #include "zend_interfaces.h"
	行  26: #include "zend_objects.h"
	行  27: #include "zend_objects_API.h"
	行  28: #include "zend_globals.h"
	行  29: #include "zend_closures_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_compile.c （匹配16次）
	行    21: #include <zend_language_parser.h>
	行    22: #include "zend.h"
	行    23: #include "zend_attributes.h"
	行    24: #include "zend_compile.h"
	行    25: #include "zend_constants.h"
	行    26: #include "zend_llist.h"
	行    27: #include "zend_API.h"
	行    28: #include "zend_exceptions.h"
	行    29: #include "zend_interfaces.h"
	行    30: #include "zend_virtual_cwd.h"
	行    31: #include "zend_multibyte.h"
	行    32: #include "zend_language_scanner.h"
	行    33: #include "zend_inheritance.h"
	行    34: #include "zend_vm.h"
	行    35: #include "zend_enum.h"
	行    36: #include "zend_observer.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_compile.h （匹配6次）
	行   23: #include "zend.h"
	行   24: #include "zend_ast.h"
	行   26: #include <stdarg.h>
	行   28: #include "zend_llist.h"
	行  761: #include "zend_globals.h"
	行  915: #include "zend_vm_opcodes.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_config.w32.h （匹配8次）
	行 23: #include <../main/config.w32.h>
	行 27: #include <malloc.h>
	行 28: #include <stdlib.h>
	行 29: #include <crtdbg.h>
	行 31: #include <string.h>
	行 36: #include <winsock2.h>
	行 37: #include <windows.h>
	行 39: #include <float.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_constants.c （匹配9次）
	行  20: #include "zend.h"
	行  21: #include "zend_constants.h"
	行  22: #include "zend_exceptions.h"
	行  23: #include "zend_execute.h"
	行  24: #include "zend_variables.h"
	行  25: #include "zend_operators.h"
	行  26: #include "zend_globals.h"
	行  27: #include "zend_API.h"
	行  28: #include "zend_constants_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_constants.h （匹配1次）
	行  23: #include "zend_globals.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_cpuinfo.c （匹配1次）
	行  19: #include "zend_cpuinfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_cpuinfo.h （匹配1次）
	行  22: #include "zend.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_default_classes.c （匹配11次）
	行 20: #include "zend.h"
	行 21: #include "zend_API.h"
	行 22: #include "zend_attributes.h"
	行 23: #include "zend_builtin_functions.h"
	行 24: #include "zend_interfaces.h"
	行 25: #include "zend_exceptions.h"
	行 26: #include "zend_closures.h"
	行 27: #include "zend_generators.h"
	行 28: #include "zend_weakrefs.h"
	行 29: #include "zend_enum.h"
	行 30: #include "zend_fibers.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_dtrace.c （匹配3次）
	行  19: #include "zend.h"
	行  20: #include "zend_API.h"
	行  21: #include "zend_dtrace.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_dtrace.h （匹配1次）
	行 38: #include <zend_dtrace_gen.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_enum.c （匹配8次）
	行  19: #include "zend.h"
	行  20: #include "zend_API.h"
	行  21: #include "zend_compile.h"
	行  22: #include "zend_enum_arginfo.h"
	行  23: #include "zend_interfaces.h"
	行  24: #include "zend_enum.h"
	行  25: #include "zend_extensions.h"
	行  26: #include "zend_observer.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_enum.h （匹配2次）
	行 22: #include "zend.h"
	行 23: #include "zend_types.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_exceptions.c （匹配10次）
	行   22: #include "zend.h"
	行   23: #include "zend_API.h"
	行   24: #include "zend_builtin_functions.h"
	行   25: #include "zend_interfaces.h"
	行   26: #include "zend_exceptions.h"
	行   27: #include "zend_vm.h"
	行   28: #include "zend_dtrace.h"
	行   29: #include "zend_smart_str.h"
	行   30: #include "zend_exceptions_arginfo.h"
	行   31: #include "zend_observer.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_exceptions.h （匹配1次）
	行 81: #include "zend_globals.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_execute.c （匹配24次）
	行   23: #include <stdio.h>
	行   24: #include <signal.h>
	行   26: #include "zend.h"
	行   27: #include "zend_compile.h"
	行   28: #include "zend_execute.h"
	行   29: #include "zend_API.h"
	行   30: #include "zend_ptr_stack.h"
	行   31: #include "zend_constants.h"
	行   32: #include "zend_extensions.h"
	行   33: #include "zend_ini.h"
	行   34: #include "zend_exceptions.h"
	行   35: #include "zend_interfaces.h"
	行   36: #include "zend_closures.h"
	行   37: #include "zend_generators.h"
	行   38: #include "zend_vm.h"
	行   39: #include "zend_dtrace.h"
	行   40: #include "zend_inheritance.h"
	行   41: #include "zend_type_info.h"
	行   42: #include "zend_smart_str.h"
	行   43: #include "zend_observer.h"
	行   44: #include "zend_system_id.h"
	行   45: #include "Optimizer/zend_func_info.h"
	行   48: #include "zend_virtual_cwd.h"
	行 5339: #include "zend_vm_execute.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_execute.h （匹配4次）
	行  24: #include "zend_compile.h"
	行  25: #include "zend_hash.h"
	行  26: #include "zend_operators.h"
	行  27: #include "zend_variables.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_execute_API.c （匹配21次）
	行   21: #include <stdio.h>
	行   22: #include <signal.h>
	行   24: #include "zend.h"
	行   25: #include "zend_compile.h"
	行   26: #include "zend_execute.h"
	行   27: #include "zend_API.h"
	行   28: #include "zend_stack.h"
	行   29: #include "zend_constants.h"
	行   30: #include "zend_extensions.h"
	行   31: #include "zend_exceptions.h"
	行   32: #include "zend_closures.h"
	行   33: #include "zend_generators.h"
	行   34: #include "zend_vm.h"
	行   35: #include "zend_float.h"
	行   36: #include "zend_fibers.h"
	行   37: #include "zend_weakrefs.h"
	行   38: #include "zend_inheritance.h"
	行   39: #include "zend_observer.h"
	行   41: #include <sys/time.h>
	行   44: #include <unistd.h>
	行   47: #include <sys/syscall.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_extensions.c （匹配2次）
	行  20: #include "zend_extensions.h"
	行  21: #include "zend_system_id.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_extensions.h （匹配2次）
	行  23: #include "zend_compile.h"
	行  24: #include "zend_build.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_fibers.c （匹配12次）
	行  20: #include "zend.h"
	行  21: #include "zend_API.h"
	行  22: #include "zend_ini.h"
	行  23: #include "zend_vm.h"
	行  24: #include "zend_exceptions.h"
	行  25: #include "zend_builtin_functions.h"
	行  26: #include "zend_observer.h"
	行  27: #include "zend_mmap.h"
	行  28: #include "zend_compile.h"
	行  29: #include "zend_closures.h"
	行  31: #include "zend_fibers.h"
	行  32: #include "zend_fibers_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_fibers.h （匹配2次）
	行  23: #include "zend_API.h"
	行  24: #include "zend_types.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_float.c （匹配3次）
	行 19: #include "zend.h"
	行 20: #include "zend_compile.h"
	行 21: #include "zend_float.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_gc.c （匹配3次）
	行   69: #include "zend.h"
	行   70: #include "zend_API.h"
	行   71: #include "zend_fibers.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_gdb.c （匹配6次）
	行  20: #include "zend.h"
	行  21: #include "zend_gdb.h"
	行  23: #include <sys/types.h>
	行  24: #include <sys/stat.h>
	行  25: #include <fcntl.h>
	行  26: #include <unistd.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_generators.c （匹配8次）
	行   20: #include "zend.h"
	行   21: #include "zend_API.h"
	行   22: #include "zend_interfaces.h"
	行   23: #include "zend_exceptions.h"
	行   24: #include "zend_generators.h"
	行   25: #include "zend_closures.h"
	行   26: #include "zend_generators_arginfo.h"
	行   27: #include "zend_observer.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_globals.h （匹配17次）
	行  24: #include <setjmp.h>
	行  25: #include <sys/types.h>
	行  27: #include "zend_globals_macros.h"
	行  29: #include "zend_atomic.h"
	行  30: #include "zend_stack.h"
	行  31: #include "zend_ptr_stack.h"
	行  32: #include "zend_hash.h"
	行  33: #include "zend_llist.h"
	行  34: #include "zend_objects.h"
	行  35: #include "zend_objects_API.h"
	行  36: #include "zend_modules.h"
	行  37: #include "zend_float.h"
	行  38: #include "zend_multibyte.h"
	行  39: #include "zend_multiply.h"
	行  40: #include "zend_arena.h"
	行  41: #include "zend_max_execution_timer.h"
	行  60: #include "zend_compile.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_hash.c （匹配3次）
	行   21: #include "zend.h"
	行   22: #include "zend_globals.h"
	行   23: #include "zend_variables.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_hash.h （匹配2次）
	行   24: #include "zend.h"
	行   25: #include "zend_sort.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_highlight.c （匹配7次）
	行  20: #include "zend.h"
	行  21: #include <zend_language_parser.h>
	行  22: #include "zend_compile.h"
	行  23: #include "zend_highlight.h"
	行  24: #include "zend_ptr_stack.h"
	行  25: #include "zend_globals.h"
	行  26: #include "zend_exceptions.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_inheritance.c （匹配13次）
	行   20: #include "zend.h"
	行   21: #include "zend_API.h"
	行   22: #include "zend_compile.h"
	行   23: #include "zend_execute.h"
	行   24: #include "zend_inheritance.h"
	行   25: #include "zend_interfaces.h"
	行   26: #include "zend_smart_str.h"
	行   27: #include "zend_operators.h"
	行   28: #include "zend_exceptions.h"
	行   29: #include "zend_enum.h"
	行   30: #include "zend_attributes.h"
	行   31: #include "zend_constants.h"
	行   32: #include "zend_observer.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_inheritance.h （匹配1次）
	行 23: #include "zend.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_ini.c （匹配10次）
	行  19: #include "zend.h"
	行  20: #include "zend_sort.h"
	行  21: #include "zend_API.h"
	行  22: #include "zend_ini.h"
	行  23: #include "zend_alloc.h"
	行  24: #include "zend_operators.h"
	行  25: #include "zend_strtod.h"
	行  26: #include "zend_modules.h"
	行  27: #include "zend_smart_str.h"
	行  28: #include <ctype.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_interfaces.c （匹配5次）
	行  19: #include "zend.h"
	行  20: #include "zend_API.h"
	行  21: #include "zend_interfaces.h"
	行  22: #include "zend_exceptions.h"
	行  23: #include "zend_interfaces_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_interfaces.h （匹配2次）
	行 22: #include "zend.h"
	行 23: #include "zend_API.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_iterators.c （匹配2次）
	行  20: #include "zend.h"
	行  21: #include "zend_API.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_list.c （匹配4次）
	行  22: #include "zend.h"
	行  23: #include "zend_list.h"
	行  24: #include "zend_API.h"
	行  25: #include "zend_globals.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_list.h （匹配2次）
	行 23: #include "zend_hash.h"
	行 24: #include "zend_globals.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_llist.c （匹配3次）
	行  20: #include "zend.h"
	行  21: #include "zend_llist.h"
	行  22: #include "zend_sort.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_long.h （匹配2次）
	行  22: #include <inttypes.h>
	行  23: #include <stdint.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_map_ptr.h （匹配1次）
	行 22: #include "zend_portability.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_max_execution_timer.c （匹配9次）
	行  19: #include <stdio.h>
	行  20: #include <signal.h>
	行  21: #include <time.h>
	行  22: #include <unistd.h>
	行  23: #include <errno.h>
	行  24: #include <sys/syscall.h>
	行  25: #include <sys/types.h>
	行  27: #include "zend.h"
	行  28: #include "zend_globals.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_max_execution_timer.h （匹配1次）
	行 22: #include "zend_long.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_mmap.h （匹配1次）
	行 18: #include "zend_portability.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_modules.h （匹配3次）
	行  23: #include "zend.h"
	行  24: #include "zend_compile.h"
	行  25: #include "zend_build.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_multibyte.c （匹配5次）
	行  20: #include "zend.h"
	行  21: #include "zend_compile.h"
	行  22: #include "zend_operators.h"
	行  23: #include "zend_multibyte.h"
	行  24: #include "zend_ini.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_multiply.h （匹配1次）
	行  20: #include "zend_portability.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_objects.c （匹配7次）
	行  21: #include "zend.h"
	行  22: #include "zend_globals.h"
	行  23: #include "zend_variables.h"
	行  24: #include "zend_API.h"
	行  25: #include "zend_interfaces.h"
	行  26: #include "zend_exceptions.h"
	行  27: #include "zend_weakrefs.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_objects.h （匹配1次）
	行 23: #include "zend.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_objects_API.c （匹配6次）
	行  21: #include "zend.h"
	行  22: #include "zend_globals.h"
	行  23: #include "zend_variables.h"
	行  24: #include "zend_API.h"
	行  25: #include "zend_objects_API.h"
	行  26: #include "zend_fibers.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_objects_API.h （匹配2次）
	行  23: #include "zend.h"
	行  24: #include "zend_compile.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_object_handlers.c （匹配12次）
	行   21: #include "zend.h"
	行   22: #include "zend_globals.h"
	行   23: #include "zend_variables.h"
	行   24: #include "zend_API.h"
	行   25: #include "zend_objects.h"
	行   26: #include "zend_objects_API.h"
	行   27: #include "zend_object_handlers.h"
	行   28: #include "zend_interfaces.h"
	行   29: #include "zend_exceptions.h"
	行   30: #include "zend_closures.h"
	行   31: #include "zend_compile.h"
	行   32: #include "zend_hash.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_observer.c （匹配4次）
	行  20: #include "zend_observer.h"
	行  22: #include "zend_extensions.h"
	行  23: #include "zend_llist.h"
	行  24: #include "zend_vm.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_observer.h （匹配3次）
	行  23: #include "zend.h"
	行  24: #include "zend_compile.h"
	行  25: #include "zend_fibers.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_opcode.c （匹配10次）
	行   21: #include <stdio.h>
	行   23: #include "zend.h"
	行   24: #include "zend_alloc.h"
	行   25: #include "zend_compile.h"
	行   26: #include "zend_extensions.h"
	行   27: #include "zend_API.h"
	行   28: #include "zend_sort.h"
	行   29: #include "zend_constants.h"
	行   30: #include "zend_observer.h"
	行   32: #include "zend_vm.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_operators.c （匹配12次）
	行   21: #include <ctype.h>
	行   23: #include "zend.h"
	行   24: #include "zend_operators.h"
	行   25: #include "zend_variables.h"
	行   26: #include "zend_globals.h"
	行   27: #include "zend_list.h"
	行   28: #include "zend_API.h"
	行   29: #include "zend_strtod.h"
	行   30: #include "zend_exceptions.h"
	行   31: #include "zend_closures.h"
	行   33: #include <locale.h>
	行   39: #include <emmintrin.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_operators.h （匹配9次）
	行  24: #include <errno.h>
	行  25: #include <math.h>
	行  26: #include <assert.h>
	行  27: #include <stddef.h>
	行  30: #include <ieeefp.h>
	行  33: #include "zend_portability.h"
	行  34: #include "zend_strtod.h"
	行  35: #include "zend_multiply.h"
	行  36: #include "zend_object_handlers.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_portability.h （匹配9次）
	行  47: #include "../TSRM/TSRM.h"
	行  49: #include <stdio.h>
	行  50: #include <assert.h>
	行  51: #include <math.h>
	行  57: #include <stdarg.h>
	行  58: #include <stddef.h>
	行  64: #include <limits.h>
	行  67: #include <intrin.h>
	行  70: #include "zend_range_check.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_ptr_stack.c （匹配3次）
	行  20: #include "zend.h"
	行  21: #include "zend_ptr_stack.h"
	行  22: #include <stdarg.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_range_check.h （匹配1次）
	行 22: #include "zend_long.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_signal.c （匹配6次）
	行  31: #include <string.h>
	行  33: #include "zend.h"
	行  34: #include "zend_globals.h"
	行  35: #include <signal.h>
	行  38: #include <unistd.h>
	行  43: #include "zend_signal.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_signal.h （匹配1次）
	行  26: #include <signal.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_smart_str.c （匹配3次）
	行  17: #include <zend.h>
	行  18: #include "zend_smart_str.h"
	行  19: #include "zend_smart_string.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_smart_str.h （匹配3次）
	行  20: #include <zend.h>
	行  21: #include "zend_globals.h"
	行  22: #include "zend_smart_str_public.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_smart_string.h （匹配3次）
	行  21: #include "zend_smart_string_public.h"
	行  23: #include <stdlib.h>
	行  24: #include <zend.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_smart_string_public.h （匹配1次）
	行 21: #include <sys/types.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_sort.c （匹配3次）
	行  20: #include "zend.h"
	行  21: #include "zend_sort.h"
	行  22: #include <limits.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_stack.c （匹配2次）
	行  20: #include "zend.h"
	行  21: #include "zend_stack.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_stream.c （匹配3次）
	行  22: #include "zend.h"
	行  23: #include "zend_compile.h"
	行  24: #include "zend_stream.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_stream.h （匹配2次）
	行  25: #include <sys/types.h>
	行  26: #include <sys/stat.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_string.c （匹配2次）
	行  19: #include "zend.h"
	行  20: #include "zend_globals.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_string.h （匹配1次）
	行  22: #include "zend.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_strtod.c （匹配12次）
	行   30:  * necessary to #include "float.h" or another system-dependent header
	行  167:  *	an environment, perhaps provided by #include "dtoa.c" in a
	行  189: #include <zend_operators.h>
	行  190: #include <zend_strtod.h>
	行  191: #include "zend_strtod_int.h"
	行  206: #include "stdlib.h"
	行  207: #include "string.h"
	行  210: #include "locale.h"
	行  215: #include <fenv.h>
	行  256: #include "errno.h"
	行  288: #include "float.h"
	行  292: #include "math.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_strtod.h （匹配2次）
	行 23: #include <zend.h>
	行 38: #include <float.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_strtod_int.h （匹配8次）
	行  23: #include <TSRM.h>
	行  26: #include <stddef.h>
	行  27: #include <stdio.h>
	行  28: #include <ctype.h>
	行  29: #include <stdarg.h>
	行  30: #include <math.h>
	行  33: #include <sys/types.h>
	行  45: #include <inttypes.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_system_id.c （匹配5次）
	行 18: #include "php.h"
	行 19: #include "zend_system_id.h"
	行 20: #include "zend_extensions.h"
	行 21: #include "ext/standard/md5.h"
	行 22: #include "ext/hash/php_hash.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_types.h （匹配3次）
	行   25: #include "zend_portability.h"
	行   26: #include "zend_long.h"
	行   27: #include <stdbool.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_type_info.h （匹配1次）
	行 22: #include "zend_types.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_variables.c （匹配7次）
	行  21: #include <stdio.h>
	行  22: #include "zend.h"
	行  23: #include "zend_API.h"
	行  24: #include "zend_ast.h"
	行  25: #include "zend_globals.h"
	行  26: #include "zend_constants.h"
	行  27: #include "zend_list.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_variables.h （匹配2次）
	行 24: #include "zend_types.h"
	行 25: #include "zend_gc.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_virtual_cwd.c （匹配18次）
	行   19: #include <sys/types.h>
	行   20: #include <sys/stat.h>
	行   21: #include <string.h>
	行   22: #include <stdio.h>
	行   23: #include <limits.h>
	行   24: #include <errno.h>
	行   25: #include <stdlib.h>
	行   26: #include <fcntl.h>
	行   27: #include <time.h>
	行   29: #include "zend.h"
	行   30: #include "zend_virtual_cwd.h"
	行   33: #include <io.h>
	行   34: #include "tsrm_win32.h"
	行   78: #include "TSRM.h"
	行   95: #include <unistd.h>
	行   97: #include <direct.h>
	行   98: #include "zend_globals.h"
	行   99: #include "zend_globals_macros.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_virtual_cwd.h （匹配14次）
	行  22: #include "TSRM.h"
	行  24: #include <sys/types.h>
	行  25: #include <sys/stat.h>
	行  26: #include <ctype.h>
	行  29: #include <utime.h>
	行  32: #include <stdarg.h>
	行  33: #include <limits.h>
	行  57: #include <unistd.h>
	行  59: #include <direct.h>
	行  63: #include <errno.h>
	行  67: #include "win32/readdir.h"
	行  68: #include <sys/utime.h>
	行  69: #include "win32/ioutil.h"
	行  89: #include <dirent.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_vm_opcodes.c （匹配3次）
	行  21: #include <stdio.h>
	行  22: #include <zend.h>
	行  23: #include <zend_vm_opcodes.h>
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_vm_trace_handlers.h （匹配1次）
	行  19: #include "zend_sort.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_vm_trace_lines.h （匹配1次）
	行 19: #include "zend_sort.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_vm_trace_map.h （匹配2次）
	行 19: #include "zend_vm_handlers.h"
	行 20: #include "zend_sort.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_weakrefs.c （匹配5次）
	行  17: #include "zend.h"
	行  18: #include "zend_interfaces.h"
	行  19: #include "zend_objects_API.h"
	行  20: #include "zend_weakrefs.h"
	行  21: #include "zend_weakrefs_arginfo.h"
  D:\\www\\程序员\\php-8.2.5-src\\php-8.2.5-src\\Zend\\zend_weakrefs.h （匹配1次）
	行 20: #include "zend_alloc.h"
CONTENT;
return $c;
}