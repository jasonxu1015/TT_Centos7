#!/bin/bash
PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:~/bin
export PATH

# Check if user is root
if [ $(id -u) != "0" ]; then
    echo "Error: You must be root to run this script, please use root to install"
    exit 1
fi

cur_dir=$(pwd)

Download_Mirror='http://soft.vpser.net'

Autoconf_Ver='autoconf-2.13'
Libiconv_Ver='libiconv-1.14'
LibMcrypt_Ver='libmcrypt-2.5.8'
Mcypt_Ver='mcrypt-2.6.8'
Mash_Ver='mhash-0.9.9.9'
Freetype_Ver='freetype-2.4.12'
Curl_Ver='curl-7.42.1'
Pcre_Ver='pcre-8.36'
Jemalloc_Ver='jemalloc-3.6.0'
Nginx_Ver='nginx-1.8.0'
Php_Ver='php-5.6.19'

TrackerBaseDir='/data/fdfs/tracker'
StorageBaseDir='/data/fdfs/storage'
DataDir='/data/fdfs/sdata'
NgxModuleDir='/data/fdfs/ngx_module'
ClientBaseDir='/data/fdfs/client'
FdfsConfDir='/etc/fdfs'
localip=""
domain=""

GetIp()
{
    read -p "Please enter local ip: " localip
}

GetDomain()
{
    read -p "Please enter domain(default is localip): " domain
    if [ "${domain}" = "" ]; then
        domain=${localip}
    fi
}

CentOS_RemoveAMP()
{
    Echo_Blue "[-] Yum remove packages..."
    rpm -qa|grep httpd
    rpm -e httpd httpd-tools
    rpm -qa|grep php
    rpm -e php-mysql php-cli php-gd php-common php

    yum -y remove httpd*
    yum -y remove php*
    yum clean all
}

CentOS_Dependent()
{
    Echo_Blue "[+] Yum installing dependent packages..."
    for packages in git wget vim make cmake gcc gcc-c++ libxml2 libxml2-devel patch wget bzip2 openssl openssl-devel libjpeg libjpeg-devel libpng libpng-devel libpng10 libpng10-devel zlib zlib-devel gd gd-devel unzip tar net-tools autoconf;
    do yum -y install $packages; done
}

Color_Text()
{
  echo -e " \e[0;$2m$1\e[0m"
}

Echo_Red()
{
  echo $(Color_Text "$1" "31")
}

Echo_Green()
{
  echo $(Color_Text "$1" "32")
}

Echo_Yellow()
{
  echo $(Color_Text "$1" "33")
}

Echo_Blue()
{
  echo $(Color_Text "$1" "34")
}

Get_OS_Bit()
{
    if [[ `getconf WORD_BIT` = '32' && `getconf LONG_BIT` = '64' ]] ; then
        Is_64bit='y'
    else
        Is_64bit='n'
    fi
}

Tar_Cd()
{
    local FileName=$1
    local DirName=$2
    cd ${cur_dir}/src
    [[ -d "${DirName}" ]] && rm -rf ${DirName}
    echo "Uncompress ${FileName}..."
    tar zxf ${FileName}
    echo "cd ${DirName}..."
    cd ${DirName}
}

Disable_Selinux()
{
    if [ -s /etc/selinux/config ]; then
        sed -i 's/SELINUX=enforcing/SELINUX=disabled/g' /etc/selinux/config
    fi
}


Download_Files()
{
    local URL=$1
    local FileName=$2
    if [ -s "${FileName}" ]; then
        echo "${FileName} [found]"
    else
        echo "Error: ${FileName} not found!!!download now..."
        wget -c ${URL}
    fi
}

Check_Download()
{
    Echo_Blue "[+] Downloading files..."
    cd ${cur_dir}/src
    Download_Files ${Download_Mirror}/lib/autoconf/${Autoconf_Ver}.tar.gz ${Autoconf_Ver}.tar.gz
    Download_Files ${Download_Mirror}/web/libiconv/${Libiconv_Ver}.tar.gz ${Libiconv_Ver}.tar.gz
    Download_Files ${Download_Mirror}/web/libmcrypt/${LibMcrypt_Ver}.tar.gz ${LibMcrypt_Ver}.tar.gz
    Download_Files ${Download_Mirror}/web/mcrypt/${Mcypt_Ver}.tar.gz ${Mcypt_Ver}.tar.gz
    Download_Files ${Download_Mirror}/web/mhash/${Mash_Ver}.tar.gz ${Mash_Ver}.tar.gz
    Download_Files ${Download_Mirror}/lib/freetype/${Freetype_Ver}.tar.gz ${Freetype_Ver}.tar.gz
    Download_Files ${Download_Mirror}/lib/curl/${Curl_Ver}.tar.gz ${Curl_Ver}.tar.gz
    Download_Files ${Download_Mirror}/web/pcre/${Pcre_Ver}.tar.gz ${Pcre_Ver}.tar.gz
    Download_Files ${Download_Mirror}/lib/jemalloc/${Jemalloc_Ver}.tar.bz2 ${Jemalloc_Ver}.tar.bz2
    Download_Files ${Download_Mirror}/web/nginx/${Nginx_Ver}.tar.gz ${Nginx_Ver}.tar.gz
    Download_Files http://cn2.php.net/distributions/${Php_Ver}.tar.gz ${Php_Ver}.tar.gz
    Download_Files 'http://nchc.dl.sourceforge.net/project/fastdfs/FastDFS%20Nginx%20Module%20Source%20Code/fastdfs-nginx-module_v1.16.tar.gz' fastdfs-nginx-module_v1.16.tar.gz
    Download_Files 'http://jaist.dl.sourceforge.net/project/fastdfs/FastDFS%20Server%20Source%20Code/FastDFS%20Server%20with%20PHP%20Extension%20Source%20Code%20V5.08/FastDFS_v5.08.tar.gz' FastDFS_v5.08.tar.gz
}


Install_Autoconf()
{
    Echo_Blue "[+] Installing ${Autoconf_Ver}"
    Tar_Cd ${Autoconf_Ver}.tar.gz ${Autoconf_Ver}
    ./configure --prefix=/usr/local/autoconf-2.13
    make && make install
}

Install_Libiconv()
{
    Echo_Blue "[+] Installing ${Libiconv_Ver}"
    Tar_Cd ${Libiconv_Ver}.tar.gz ${Libiconv_Ver}
    patch -p0 < ${cur_dir}/patch/libiconv-glibc-2.16.patch
    ./configure --enable-static
    make && make install
}

Install_Libmcrypt()
{
    Echo_Blue "[+] Installing ${LibMcrypt_Ver}"
    Tar_Cd ${LibMcrypt_Ver}.tar.gz ${LibMcrypt_Ver}
    ./configure
    make && make install
    /sbin/ldconfig
    cd libltdl/
    ./configure --enable-ltdl-install
    make && make install
    ln -s /usr/local/lib/libmcrypt.la /usr/lib/libmcrypt.la
    ln -s /usr/local/lib/libmcrypt.so /usr/lib/libmcrypt.so
    ln -s /usr/local/lib/libmcrypt.so.4 /usr/lib/libmcrypt.so.4
    ln -s /usr/local/lib/libmcrypt.so.4.4.8 /usr/lib/libmcrypt.so.4.4.8
    ldconfig
}

Install_Mcrypt()
{
    Echo_Blue "[+] Installing ${Mcypt_Ver}"
    Tar_Cd ${Mcypt_Ver}.tar.gz ${Mcypt_Ver}
    ./configure
    make && make install
}

Install_Mhash()
{
    Echo_Blue "[+] Installing ${Mash_Ver}"
    Tar_Cd ${Mash_Ver}.tar.gz ${Mash_Ver}
    ./configure
    make && make install
    ln -s /usr/local/lib/libmhash.a /usr/lib/libmhash.a
    ln -s /usr/local/lib/libmhash.la /usr/lib/libmhash.la
    ln -s /usr/local/lib/libmhash.so /usr/lib/libmhash.so
    ln -s /usr/local/lib/libmhash.so.2 /usr/lib/libmhash.so.2
    ln -s /usr/local/lib/libmhash.so.2.0.1 /usr/lib/libmhash.so.2.0.1
    ldconfig
}

Install_Freetype()
{
    Echo_Blue "[+] Installing ${Freetype_Ver}"
    Tar_Cd ${Freetype_Ver}.tar.gz ${Freetype_Ver}
     
    make && make install

    cat > /etc/ld.so.conf.d/freetype.conf<<EOF
/usr/local/freetype/lib
EOF
    ldconfig
    ln -sf /usr/local/freetype/include/freetype2 /usr/local/include
    ln -sf /usr/local/freetype/include/ft2build.h /usr/local/include
}

Install_Curl()
{
    Echo_Blue "[+] Installing ${Curl_Ver}"
    Tar_Cd ${Curl_Ver}.tar.gz ${Curl_Ver}
    ./configure --prefix=/usr/local/curl --enable-ares
    make && make install
}

Install_Pcre()
{
    Cur_Pcre_Ver=`pcre-config --version`
    if echo "${Cur_Pcre_Ver}" | grep -vEqi '^8.';then
        Echo_Blue "[+] Installing ${Pcre_Ver}"
        Tar_Cd ${Pcre_Ver}.tar.gz ${Pcre_Ver}
        ./configure
        make && make install
    fi
}

Install_Jemalloc()
{
    Echo_Blue "[+] Installing ${Jemalloc_Ver}"
    cd ${cur_dir}/src
    tar jxf ${Jemalloc_Ver}.tar.bz2
    cd ${Jemalloc_Ver}
    ./configure
    make && make install
    ldconfig
}


CentOS_Lib_Opt()
{
    if [ "${Is_64bit}" = "y" ] ; then
    ln -s /usr/lib64/libpng.* /usr/lib/
    ln -s /usr/lib64/libjpeg.* /usr/lib/
    fi

    ulimit -v unlimited

    if [ `grep -L "/lib"    '/etc/ld.so.conf'` ]; then
        echo "/lib" >> /etc/ld.so.conf
    fi

    if [ `grep -L '/usr/lib'    '/etc/ld.so.conf'` ]; then
        echo "/usr/lib" >> /etc/ld.so.conf
        #echo "/usr/lib/openssl/engines" >> /etc/ld.so.conf
    fi

    if [ -d "/usr/lib64" ] && [ `grep -L '/usr/lib64'    '/etc/ld.so.conf'` ]; then
        echo "/usr/lib64" >> /etc/ld.so.conf
        #echo "/usr/lib64/openssl/engines" >> /etc/ld.so.conf
    fi

    if [ `grep -L '/usr/local/lib'    '/etc/ld.so.conf'` ]; then
        echo "/usr/local/lib" >> /etc/ld.so.conf
    fi

    ldconfig

    cat >>/etc/security/limits.conf<<eof
* soft nproc 65535
* hard nproc 65535
* soft nofile 65535
* hard nofile 65535
eof

    echo "fs.file-max=65535" >> /etc/sysctl.conf
}

Export_PHP_Autoconf()
{
    export PHP_AUTOCONF=/usr/local/autoconf-2.13/bin/autoconf
    export PHP_AUTOHEADER=/usr/local/autoconf-2.13/bin/autoheader
}

Ln_PHP_Bin()
{
    ln -sf /usr/local/php/bin/php /usr/bin/php
    ln -sf /usr/local/php/bin/phpize /usr/bin/phpize
    ln -sf /usr/local/php/bin/pear /usr/bin/pear
    ln -sf /usr/local/php/bin/pecl /usr/bin/pecl
    ln -sf /usr/local/php/sbin/php-fpm /usr/bin/php-fpm
}
Pear_Pecl_Set()
{
    pear config-set php_ini /usr/local/php/etc/php.ini
    pecl config-set php_ini /usr/local/php/etc/php.ini
}
Install_PHP_56()
{
    Echo_Blue "[+] Installing ${Php_Ver}"
    Tar_Cd ${Php_Ver}.tar.gz ${Php_Ver}
    ./configure --prefix=/usr/local/php --with-config-file-path=/usr/local/php/etc --enable-fpm --with-fpm-user=www --with-fpm-group=www --with-mysql=mysqlnd --with-mysqli=mysqlnd --with-pdo-mysql=mysqlnd --with-iconv-dir --with-freetype-dir=/usr/local/freetype --with-jpeg-dir --with-png-dir --with-zlib --with-libxml-dir=/usr --enable-xml --disable-rpath --enable-bcmath --enable-shmop --enable-sysvsem --enable-inline-optimization --with-curl --enable-mbregex --enable-mbstring --with-mcrypt --enable-ftp --with-gd --enable-gd-native-ttf --with-openssl --with-mhash --enable-pcntl --enable-sockets --with-xmlrpc --enable-zip --enable-soap --with-gettext --disable-fileinfo --enable-opcache

    make ZEND_EXTRA_LIBS='-liconv'
    make install

    Ln_PHP_Bin

    echo "Copy new php configure file..."
    mkdir -p /usr/local/php/etc
    \cp php.ini-production /usr/local/php/etc/php.ini

    cd ${cur_dir}
    # php extensions
    echo "Modify php.ini......"
    sed -i 's/post_max_size = 8M/post_max_size = 50M/g' /usr/local/php/etc/php.ini
    sed -i 's/upload_max_filesize = 2M/upload_max_filesize = 50M/g' /usr/local/php/etc/php.ini
    sed -i 's/;date.timezone =/date.timezone = PRC/g' /usr/local/php/etc/php.ini
    sed -i 's/short_open_tag = Off/short_open_tag = On/g' /usr/local/php/etc/php.ini
    sed -i 's/; cgi.fix_pathinfo=1/cgi.fix_pathinfo=0/g' /usr/local/php/etc/php.ini
    sed -i 's/; cgi.fix_pathinfo=0/cgi.fix_pathinfo=0/g' /usr/local/php/etc/php.ini
    sed -i 's/;cgi.fix_pathinfo=1/cgi.fix_pathinfo=0/g' /usr/local/php/etc/php.ini
    sed -i 's/max_execution_time = 30/max_execution_time = 300/g' /usr/local/php/etc/php.ini
    sed -i 's/disable_functions =.*/disable_functions = passthru,exec,system,chroot,scandir,chgrp,chown,shell_exec,proc_open,proc_get_status,popen,ini_alter,ini_restore,dl,openlog,syslog,readlink,symlink,popepassthru,stream_socket_server/g' /usr/local/php/etc/php.ini
    Pear_Pecl_Set

    echo "Install ZendGuardLoader for PHP 5.6..."
    cd ${cur_dir}/src
    if [ "${Is_64bit}" = "y" ] ; then
        Download_Files ${Download_Mirror}/web/zend/zend-loader-php5.6-linux-x86_64.tar.gz
        tar zxf zend-loader-php5.6-linux-x86_64.tar.gz
        mkdir -p /usr/local/zend/
        \cp zend-loader-php5.6-linux-x86_64/ZendGuardLoader.so /usr/local/zend/
    else
        Download_Files ${Download_Mirror}/web/zend/zend-loader-php5.6-linux-i386.tar.gz
        tar zxf zend-loader-php5.6-linux-i386.tar.gz
        mkdir -p /usr/local/zend/
        \cp zend-loader-php5.6-linux-i386/ZendGuardLoader.so /usr/local/zend/
    fi

    echo "Write ZendGuardLoader to php.ini..."
cat >>/usr/local/php/etc/php.ini<<EOF

;eaccelerator

;ionCube

[Zend ZendGuard Loader]
zend_extension=/usr/local/zend/ZendGuardLoader.so
zend_loader.enable=1
zend_loader.disable_licensing=0
zend_loader.obfuscation_level_support=3
zend_loader.license_path=

;opcache
[Zend Opcache]
zend_extension=opcache.so
opcache.memory_consumption=128
opcache.interned_strings_buffer=8
opcache.max_accelerated_files=4000
opcache.revalidate_freq=60
opcache.fast_shutdown=1
opcache.enable_cli=1
;opcache end

;xcache
;xcache end
EOF

echo "Copy Opcache Control Panel..."

    echo "Creating new php-fpm configure file..."
    cat >/usr/local/php/etc/php-fpm.conf<<EOF
[global]
pid = /usr/local/php/var/run/php-fpm.pid
error_log = /usr/local/php/var/log/php-fpm.log
log_level = notice

[www]
listen = /tmp/php-cgi.sock
listen.backlog = -1
listen.allowed_clients = 127.0.0.1
listen.owner = www
listen.group = www
listen.mode = 0666
user = www
group = www
pm = dynamic
pm.max_children = 10
pm.start_servers = 2
pm.min_spare_servers = 1
pm.max_spare_servers = 6
request_terminate_timeout = 100
request_slowlog_timeout = 0
slowlog = var/log/slow.log
EOF

    echo "Copy php-fpm init.d file..."
    \cp ${cur_dir}/src/${Php_Ver}/sapi/fpm/init.d.php-fpm /etc/init.d/php-fpm
    chmod +x /etc/init.d/php-fpm
}

Install_Nginx()
{
    Echo_Blue "[+] Installing ${Nginx_Ver}... "
    groupadd www
    useradd -s /sbin/nologin -g www www

    Tar_Cd ${Nginx_Ver}.tar.gz ${Nginx_Ver}
    ./configure --user=www --group=www --prefix=/usr/local/nginx --with-http_stub_status_module --with-http_ssl_module --with-http_spdy_module --with-http_gzip_static_module --with-ipv6 --with-http_sub_module --with-ld-opt='-ljemalloc' --add-module=${cur_dir}/src/fastdfs-nginx-module/src
    make && make install
    cd ../

    ln -sf /usr/local/nginx/sbin/nginx /usr/bin/nginx

    rm -f /usr/local/nginx/conf/nginx.conf
    cd ${cur_dir}
    \cp conf/nginx.conf /usr/local/nginx/conf/nginx.conf
    \cp conf/rewrite/dabr.conf /usr/local/nginx/conf/dabr.conf
    \cp conf/rewrite/discuz.conf /usr/local/nginx/conf/discuz.conf
    \cp conf/rewrite/sablog.conf /usr/local/nginx/conf/sablog.conf
    \cp conf/rewrite/typecho.conf /usr/local/nginx/conf/typecho.conf
    \cp conf/rewrite/typecho2.conf /usr/local/nginx/conf/typecho2.conf
    \cp conf/rewrite/wordpress.conf /usr/local/nginx/conf/wordpress.conf
    \cp conf/rewrite/discuzx.conf /usr/local/nginx/conf/discuzx.conf
    \cp conf/rewrite/discuzx2.conf /usr/local/nginx/conf/discuzx2.conf
    \cp conf/rewrite/none.conf /usr/local/nginx/conf/none.conf
    \cp conf/rewrite/wp2.conf /usr/local/nginx/conf/wp2.conf
    \cp conf/rewrite/phpwind.conf /usr/local/nginx/conf/phpwind.conf
    \cp conf/rewrite/shopex.conf /usr/local/nginx/conf/shopex.conf
    \cp conf/rewrite/dedecms.conf /usr/local/nginx/conf/dedecms.conf
    \cp conf/rewrite/drupal.conf /usr/local/nginx/conf/drupal.conf
    \cp conf/rewrite/ecshop.conf /usr/local/nginx/conf/ecshop.conf
    \cp conf/pathinfo.conf /usr/local/nginx/conf/pathinfo.conf
    \cp conf/enable-php.conf /usr/local/nginx/conf/enable-php.conf
    \cp conf/enable-php-pathinfo.conf /usr/local/nginx/conf/enable-php-pathinfo.conf
    \cp conf/proxy-pass-php.conf /usr/local/nginx/conf/proxy-pass-php.conf

    mkdir -p /home/wwwroot/default
    chmod +w /home/wwwroot/default
    mkdir -p /home/wwwlogs
    chmod 777 /home/wwwlogs

    chown -R www:www /home/wwwroot/default

    mkdir /usr/local/nginx/conf/vhost

    cat >/home/wwwroot/default/.user.ini<<EOF
open_basedir=/home/wwwroot/default:/tmp/:/proc/
EOF
    chmod 644 /home/wwwroot/default/.user.ini
    chattr +i /home/wwwroot/default/.user.ini

    \cp init.d/init.d.nginx /etc/init.d/nginx
    chmod +x /etc/init.d/nginx

    sed -i "s/server_name localhost/server_name ${domain}/g" /usr/local/nginx/conf/nginx.conf
    \cp ${cur_dir}/conf/index.php  /home/wwwroot/default/
    chown -R www:www /home/wwwroot/default/index.php
}

Install_Fastdfs()
{
    groupadd fdfs
    useradd -s /sbin/nologin -g fdfs fdfs

    mkdir -p ${TrackerBaseDir}
    mkdir -p ${StorageBaseDir}
    mkdir -p ${DataDir}
    mkdir -p ${NgxModuleDir}
    mkdir -p ${ClientBaseDir}


    chown -R fdfs:fdfs /data/
    chown -R www:www /data/fdfs/ngx_module/

    cd ${cur_dir}/src
    git clone https://github.com/happyfish100/libfastcommon.git
    cd libfastcommon
    ./make.sh
    ./make.sh install

    cd ${cur_dir}/src
    Tar_Cd FastDFS_v5.08.tar.gz FastDFS
    sed -i "s/#define FDFS_FILE_EXT_NAME_MAX_LEN\t6/#define FDFS_FILE_EXT_NAME_MAX_LEN\t15/g" common/fdfs_global.h
    ./make.sh
    ./make.sh install
    cd php_client
    /usr/local/php/bin/phpize
    ./configure --with-php-config=/usr/local/php/bin/php-config
    make && make install
    cat fastdfs_client.ini >> /usr/local/php/etc/php.ini
    sed -i "s/fastdfs_client.base_path = \/tmp/fastdfs_client.base_path = \/data\/fdfs\/client/" /usr/local/php/etc/php.ini
    \cp ${cur_dir}/src/FastDFS/conf/http.conf ${FdfsConfDir}
    \cp ${cur_dir}/src/FastDFS/conf/mime.types ${FdfsConfDir}


    cd ${cur_dir}/src
    Tar_Cd fastdfs-nginx-module_v1.16.tar.gz fastdfs-nginx-module
    sed -i "s/usr\/local\/include/usr\/include/g" ${cur_dir}/src/fastdfs-nginx-module/src/config
    \cp ${cur_dir}/conf/mod_fastdfs.conf /etc/fdfs/




    cd ${FdfsConfDir}

    \cp ${cur_dir}/conf/tracker.conf tracker.conf

    sed -i "s/bind_addr=/bind_addr=${localip}/" tracker.conf
    sed -i "s/base_path=/base_path=\/data\/fdfs\/tracker/" tracker.conf
    sed -i "s/log_level=info/log_level=warn/" tracker.conf
    sed -i "s/run_by_group=/run_by_group=fdfs/" tracker.conf
    sed -i "s/run_by_user=/run_by_user=fdfs/" tracker.conf

    \cp ${cur_dir}/conf/storage.conf storage.conf

    sed -i "s/bind_addr=/bind_addr=${localip}/" storage.conf
    sed -i "s/base_path=/base_path=\/data\/fdfs\/storage/" storage.conf
    sed -i "s/store_path0=/store_path0=\/data\/fdfs\/sdata/" storage.conf
    sed -i "s/tracker_server=/tracker_server=${localip}:22122/" storage.conf
    sed -i "s/log_level=info/log_level=warn/" storage.conf
    sed -i "s/run_by_group=/run_by_group=fdfs/" storage.conf
    sed -i "s/run_by_user=/run_by_user=fdfs/" storage.conf



    \cp ${cur_dir}/conf/client.conf client.conf

    sed -i "s/base_path=/base_path=\/data\/fdfs\/client/" client.conf
    sed -i "s/tracker_server=/tracker_server=${localip}:22122/" client.conf
    sed -i "s/log_level=info/log_level=warn/" storage.conf

    sed -i "s/base_path=/base_path=\/data\/fdfs\/ngx_module/" mod_fastdfs.conf
    sed -i "s/store_path0=/store_path0=\/data\/fdfs\/sdata/" mod_fastdfs.conf
    sed -i "s/tracker_server=/tracker_server=${localip}:22122/" mod_fastdfs.conf
    sed -i "s/log_level=info/log_level=warn/" mod_fastdfs.conf
    sed -i "s/logfile_name=/log_filename=\/data\/fdfs\/ngx_module\/ngx_mod.log/" mod_fastdfs.conf
    sed -i "s/url_have_group_name = false/url_have_group_name = true/" mod_fastdfs.conf


    cd ${cur_dir}
}

Start()
{
    /usr/bin/fdfs_trackerd /etc/fdfs/tracker.conf
    /usr/bin/fdfs_storaged /etc/fdfs/storage.conf
    /etc/init.d/php-fpm start
    /etc/init.d/nginx start
}

GetIp
GetDomain

CentOS_RemoveAMP
CentOS_Dependent
Disable_Selinux
Check_Download
Install_Autoconf
Install_Libiconv
Install_Libmcrypt
Install_Mhash
Install_Mcrypt
Install_Freetype
Install_Curl
Install_Pcre
Install_Jemalloc
CentOS_Lib_Opt
Export_PHP_Autoconf
Get_OS_Bit
Install_PHP_56
Install_Fastdfs
Install_Nginx
Start