#!/bin/bash
set -e
echo "Preparing test enviroment"
rm -rf test.wd
mkdir -p test.wd test.wd/upload test.db test.wd/cgi-bin
cat >test.wd/config.xml <<DELIM
<?xml version="1.0" ?>
<boinc>
	<config>
		<daily_result_quota>10240</daily_result_quota>
		<one_result_per_user_per_wu>0</one_result_per_user_per_wu>
		<one_result_per_host_per_wu>1</one_result_per_host_per_wu>
		<max_wus_to_send>1024</max_wus_to_send>
		<send_result_abort>1</send_result_abort>
		<resend_lost_results>1</resend_lost_results>
		<min_sendwork_interval>30</min_sendwork_interval>
		<next_rpc_delay>86400</next_rpc_delay>
		<upload_dir>$PWD/test.wd/upload</upload_dir>
		<disable_account_creation_rpc>0</disable_account_creation_rpc>
		<long_name>T.Brada Experiment Grid</long_name>
		<sched_debug_level>99</sched_debug_level>
		<cache_md5_info>1</cache_md5_info>
		<upload_url>https://boinc.tbrada.eu/tbrada_cgi/fuh</upload_url>
		<disable_account_creation>0</disable_account_creation>
		<disable_web_account_creation>0</disable_web_account_creation>
		<download_url>https://boinc.tbrada.eu/download</download_url>
		<db_user>boinc</db_user>
		<log_dir>$PWD/test.wd/log</log_dir>
		<enable_delete_account>0</enable_delete_account>
		<app_dir>$PWD/testq/apps</app_dir>
		<download_dir>$PWD/testq/download</download_dir>
		<fuh_debug_level>3</fuh_debug_level>
		<master_url>https://boinc.tbrada.eu/</master_url>
		<host>saran</host>
		<db_name>boinc</db_name>
		<shmem_key>0x1111b896</shmem_key>
		<show_results>1</show_results>
		<key_dir>$PWD/testq/keys/</key_dir>
		<dont_generate_upload_certificates>0</dont_generate_upload_certificates>
		<enable_privacy_by_default>0</enable_privacy_by_default>
		<enable_login_mustagree_termsofuse>0</enable_login_mustagree_termsofuse>
		<ignore_upload_certificates>1</ignore_upload_certificates>
		<db_passwd></db_passwd>
		<db_user>root</db_user>
		<db_host></db_host>
		<db_socket>$PWD/test.db/mysqld.sock</db_socket>
		<profile_min_credit>4000</profile_min_credit>
	</config>
</boinc>
DELIM
if [ ! -f test.db/my.cnf ]; then
echo "Creating database config"
cat >test.db/my.cnf <<DELIM
[client]
user = root
socket          = $PWD/test.db/mysqld.sock
[mysqld]
datadir = $PWD/test.db
port            = 3306
socket          = $PWD/test.db/mysqld.sock
skip-external-locking
key_buffer_size = 8M
max_allowed_packet = 1M
table_open_cache = 64
sort_buffer_size = 512K
net_buffer_length = 8K
read_buffer_size = 256K
read_rnd_buffer_size = 512K
myisam_sort_buffer_size = 1M
#tmpdir         = /tmp/ ????
skip-networking
net-read-timeout = 300
net-write-timeout = 300
default-storage-engine=InnoDB
default-tmp-storage-engine=InnoDB
innodb_buffer_pool_chunk_size=16M
innodb_buffer_pool_size = 128M
skip-log-bin
character-set-server=utf8
collation-server=utf8_general_ci
#innodb-file-per-table=OFF
log_warnings=0
innodb_flush_log_at_trx_commit = 2
innodb_flush_log_at_timeout = 60
innodb_doublewrite = 0
DELIM
mariadb-install-db --defaults-file=test.db/my.cnf --auth-root-authentication-method=normal
else
echo "Using mariadb in test.db directory"
fi
mariadbd=1
function finish {
	echo "Stopping mariadb"
  kill $mariadbd
  wait $mariadbd
}
trap finish EXIT
echo "Starting mariadb"
/usr/bin/mariadbd --defaults-file=test.db/my.cnf 2>test.db/mysqld.log &
sleep 2
mariadbd=$!

echo "Reloading database contents"
mysql --defaults-file=test.db/my.cnf -B <testq/database.sql

cd test.wd

function do_test {
	set +e
	echo "Running test $1"
	if ! valgrind ../schedq --stdio <../testq/$1.inp >$1.out; then
		echo "schedq exited with $?"
	fi
	if ! diff -u ../testq/$1.out $1.out; then
		echo "Response mismatch from test $1"
	fi
}

do_test sample

