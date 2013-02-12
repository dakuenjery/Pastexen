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
	
	final class ApplicationUtils {
		public static function urlStripParameter($url, $parameter) {
			$parsedUrl = parse_url($url);
			
			if(!isset($parsedUrl['query']))
				return $url;

			$querySplitted = explode('&', $parsedUrl['query']);
			foreach($querySplitted as $queryElement) {
				list($name, $value) = explode('=', $queryElement);
				if($value == null)
					$value = '';
				$queryData[$name] = $value;
			}
			
			if(!isset($queryData[$parameter]))
				return $url;
			
			unset($queryData[$parameter]);
			
			$url = $parsedUrl['scheme'] . '://' . $parsedUrl['host'] . $parsedUrl['path'] . '?' . http_build_query($queryData);
			return $url;
		}
	}