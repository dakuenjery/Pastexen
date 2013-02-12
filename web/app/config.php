<?php
	/*
	 * Pastexen web frontend - https://github.com/bakwc/Pastexen
	 * Copyright (C) 2013 powder96 <https://github.com/powder96>
	 *
	 * This program is free software: you can redistribute it and/or modify
	 * it under the terms of the GNU General Public License as published by
	 * the Free Software Foundation, either version 3 of the License, or
	 * (at your option) any later version.
	 *
	 * This program is distributed in the hope that it will be useful,
	 * but WITHOUT ANY WARRANTY; without even the implied warranty of
	 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	 * GNU General Public License for more details.
	 *
	 * You should have received a copy of the GNU General Public License
	 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
	 */
	
	if(!defined('APPLICATION_ENTRY_POINT')) {
		echo 'Access denied.';
		exit();
	}
	
	$applicationConfig = array(
		'database_host'					=> '127.0.0.1',
		'database_port'					=> 6379,
		'database_password'				=> null,
		
		'language_default'				=> 'ru',
		
		'download_link_windows'			=> 'https://github.com/bakwc/Pastexen/raw/master/builds/pastexen_0.2_win32_installer.exe',
		'download_link_other'			=> 'https://github.com/bakwc/Pastexen/tree/master/builds',
		'download_link_source'			=> 'https://github.com/bakwc/Pastexen/',
		
		'user_password_salt'			=> 'pastexen',
		
		'file_image_link'				=> 'http://pastexen.com/i/%s',
		'file_source_link'				=> 'http://pastexen.com/code.php?file=%s',
		'file_image_dir'				=> dirname(__FILE__) . '/../i',
		'file_source_dir'				=> dirname(__FILE__) . '/../s',
		'file_thumbnail_min_size'		=> 24,
		'file_thumbnail_max_size'		=> 64,
		'file_source_thumbnail_bgcolor'	=> array(  0,   0,   0),
		'file_source_thumbnail_fgcolor'	=> array(255, 255, 255),
		'file_image_thumbnail_bgcolor'	=> array(  0,   0,   0)
	);