<?php
$allowtype = array(
	'gif', // for image
	'jpg', // for image
	'bmp', // for image
	'png', // for image
	'ico', // for image
	'audio', // for audio
);

function upload($filepath, $extname) {
	$tracker = fastdfs_tracker_get_connection();
	if (!$tracker) {
		return false;
	}
	$storage = fastdfs_tracker_query_storage_store();
	if (!$storage) {
		return false;
	}

	$server = fastdfs_connect_server($storage['ip_addr'], $storage['port']);
	if (!fastdfs_active_test($server)) {
		return false;
	}
	$storage['sock'] = $server['sock'];
	return fastdfs_storage_upload_by_filename($filepath, $extname, array(), null, $tracker, $storage);
}
$method = $_SERVER['REQUEST_METHOD'];
if ($method == "GET") {
	echo "It works!";
	exit();
} else {
	$filename = $_FILES["file"]["name"];
	$filetype = $_FILES["file"]["type"];
	$filesize = $_FILES["file"]["size"];
	$filepath = $_FILES["file"]["tmp_name"];
	$extname = pathinfo($filename, PATHINFO_EXTENSION);
	//file_put_contents("file.log", 'name:'.$filename.';type:'.$filetype.';extname:'.$extname.';size:'.$filesize."\n",FILE_APPEND);
	$ret = array(
		'error_code' => 3,
		'error_msg' => 'Unknown error',
		'path' => '',
		'url' => '',
	);
	//图片最大值10M
	if ($filesize > 20971520) {
		$ret['error_code'] = 1;
		$ret['error_msg'] = 'file is too big.';
	} else {
		if (in_array($extname, $allowtype)) {
			$realext = $extname;
			if ($extname == 'gz') {
				if (strpos($filename, 'tar.gz')) {
					$realext = 'tar.gz';
				}
			} else {
				//获取$filename = xxxx_123x456.jpg这种格式
				$pos = strrpos($filename, '_');
				$posdot = strpos($filename, '.');
				if ($pos && $posdot && $posdot > $posx) {
					$meta = substr($filename, $pos + 1, $posdot - $pos);
					//$meta = 123x456
					$posx = strpos($meta, 'x');
					if ($posx) {
						$width = substr($meta, 0, $posx);
						$height = substr($meta, $posx + 1);
						$realext = $width . 'x' . $height . $extname;
					}
				}
			}
			$fileinfo = upload($filepath, $realext);
			if ($fileinfo) {
				$ret['error_code'] = 0;
				$ret['error_msg'] = 'success';
				$ret['path'] = '/' . $fileinfo['group_name'] . '/' . $fileinfo['filename'];
				$accesshost = $_SERVER['SERVER_NAME'];
				$accessport = $_SERVER["SERVER_PORT"];
				if ($accessport == 80) {
					$ret['url'] = 'http://' . $accesshost . $ret['path'];
				} else {
					$ret['url'] = 'http://' . $accesshost . ':' . $accessport . $ret['path'];
				}

			} else {
				$ret['error_code'] = 4;
				$ret['error_msg'] = 'Internal error';
			}

		} else {
			//文件类型不对
			$ret['error_code'] = 2;
			$ret['error_msg'] = 'unsupport file';
		}

	}

	echo json_encode($ret);
}
?>