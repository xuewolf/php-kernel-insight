
# 如何厘清include关系呢？
# 所有有include 都是相对于main目录或本目录，两个地方都可以引用到
	#php.h 是同一个，config.h不是
	
次数大于10的include,141个，20次以上65个


php.h	408
	<dmalloc.h>
	php_version.h
	zend.h
	zend_sort.h
	php_compat.h
	zend_API.h
	<assert.h>
	<unix.h>
	<alloca.h>
	<build-defs.h>
	<stdlib.h>
	<ctype.h>
	<unistd.h>
	<stdarg.h>
	zend_hash.h
	zend_alloc.h
	zend_stack.h
	<string.h>
	win32/param.h
	<pwd.h>
	<sys/param.h>
	<limits.h>
	snprintf.h
	spprintf.h
	php_syslog.h
	main/php_output.h
	php_streams.h
	php_memory_streams.h
	fopen_wrappers.h
	zend_virtual_cwd.h
	zend_constants.h
	php_reentrancy.h
config.h	227
	# 不同
	
<stdio.h>	174
<stdlib.h>	168
<string.h>	142

php_ini.h	130
	zend_ini.h

ext/standard/info.h	122
	#0 无引用
	
<sys/types.h>	99

zend.h	96
	zend_types.h
	zend_map_ptr.h
	zend_errors.h
	zend_alloc.h
	zend_llist.h
	zend_string.h
	zend_hash.h
	zend_ast.h
	zend_gc.h
	zend_variables.h
	zend_iterators.h
	zend_stream.h
	zend_smart_str_public.h
	zend_smart_string_public.h
	zend_signal.h
	zend_max_execution_timer.h
	zend_object_handlers.h
	zend_operators.h

mbfilter.h	85
	# mbstring 用的东西

<unistd.h>	84
zend_exceptions.h	76
	# zend_globals.h
	
<fcntl.h>	72
<errno.h>	69
<ctype.h>	64
<sys/stat.h>	63

zend_smart_str.h	62
	# zend_globals.h
	# zend_smart_str_public.h

<stdarg.h>	56
SAPI.h	56
	php.h
	zend.h
	zend_API.h
	zend_llist.h
	zend_operators.h
	
php_globals.h	56
	zend_globals.h
	
php_version.h	53
	# 无引用
	
<stddef.h>	53
<php.h>	53
gd.h	52

<math.h>	50
zend_API.h	50
	zend_modules.h
	zend_list.h
	zend_operators.h
	zend_variables.h
	zend_execute.h
	zend_type_info.h
	
zend_interfaces.h	43
	zend.h
	zend_API.h
	
<time.h>	42
TSRM.h	40
	# 只引用内部库
	
zend_compile.h	39
	zend.h
	zend_ast.h
	<stdarg.h>
	zend_llist.h
	zend_globals.h
	zend_vm_opcodes.h
	
ext/standard/php_string.h	36
<signal.h>	35
php_intl.h	35
zend_globals.h	33
	<setjmp.h>
	<sys/types.h>
	zend_globals_macros.h
	zend_atomic.h
	zend_stack.h
	zend_ptr_stack.h
	zend_hash.h
	zend_llist.h
	zend_objects.h
	zend_objects_API.h
	zend_modules.h
	zend_float.h
	zend_multibyte.h
	zend_multiply.h
	zend_arena.h
	zend_max_execution_timer.h
	zend_compile.h
	
ext/standard/file.h	31
<limits.h>	30
php_network.h	28
ext/standard/basic_functions.h	28
zend_execute.h	28
zend_extensions.h	28
<sys/time.h>	27
pcre2_internal.h	26
Zend/zend_exceptions.h	25
file.h	25
<stdint.h>	25
zend_constants.h	25
fpm_config.h	25
zlog.h	24
intl_error.h	23
intl_convert.h	23
zend_vm.h	23
	# 无引用
php_dom.h	22
gdhelpers.h	22
mysqlnd.h	22
mysqlnd_debug.h	22
<config.h>	21
bcmath.h	21
private.h	21
zend_attributes.h	21
<unicode/ustring.h>	21
mysqlnd_priv.h	21
pdo/php_pdo.h	21
pdo/php_pdo_driver.h	21
php_main.h	20
php_variables.h	20
phpdbg.h	20
<windows.h>	19
zend_observer.h	18
zend_operators.h	18
	zend_portability.h
	zend_strtod.h
	zend_multiply.h
	zend_object_handlers.h
	
php_spl.h	18
php_string.h	18
fpm_worker_pool.h	18
ext/random/php_random.h	17
<sys/socket.h>	17
<locale.h>	17
<assert.h>	16
<netinet/in.h>	16
ext/standard/php_var.h	16
php_hash.h	16
Optimizer/zend_optimizer_internal.h	16
<winsock2.h>	15
<inttypes.h>	15
ext/standard/head.h	15
zend_hash.h	15
fpm.h	15
win32/winutil.h	14
php_dba.h	14
fopen_wrappers.h	14
magic.h	14
<arpa/inet.h>	14
intl_data.h	14
ext/standard/php_standard.h	14
zend_bitset.h	14
zend_ini.h	14
zend_closures.h	13
ext/pcre/php_pcre.h	13
<netdb.h>	13
zend_shared_alloc.h	13
Optimizer/zend_optimizer.h	13
phpdbg_utils.h	13
<io.h>	12
<sys/param.h>	12
<zend.h>	12
unicode_table_jis.h	12
mysqlnd_wireprotocol.h	12
zend_sort.h	12
ZendAccelerator.h	12
zend_modules.h	12
basic_functions.h	12
../fpm_config.h	12
../fpm_events.h	12
fpm_cleanup.h	12
phpdbg_cmd.h	12
php_com_dotnet.h	11
php_com_dotnet_internal.h	11
php_streams.h	11
win32/time.h	11
<sys/mman.h>	11
<sys/wait.h>	11
php_open_temporary_file.h	11
<process.h>	11
zend_variables.h	11
unicode_table_cp932_ext.h	11
mysqlnd_statistics.h	11
zend_fibers.h	11
zend_highlight.h	11
fpm_scoreboard.h	11
ext/standard/url.h	10
timelib.h	10
ext/libxml/php_libxml.h	10
win32/param.h	10
gd_errors.h	10
KeccakSponge.inc	10
<unicode/utypes.h>	10
intl_common.h	10
zend_enum.h	10
mysqlnd_connection.h	10
zend_virtual_cwd.h	10
fpm_clock.h	10
phpdbg_prompt.h	10
zend_ssa.h	10
zend_inference.h	10