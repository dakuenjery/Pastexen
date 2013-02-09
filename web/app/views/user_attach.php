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
	
	$pageTitle = $this->l('account_attach_title');
	require(dirname(__FILE__) . '/includes/header.php');
?>

<h1><?php echo $this->l('account_attach_title'); ?></h1>
<hr />
<?php
	if(isset($success) && !$success) {
		echo '<div class="alert alert-block alert-error">';
		echo '<h4>' . $this->l('account_attach_errors_occured') . '</h4>';
		if($uuidBad)
			echo '<br />' . $this->l('error_uuid_bad');
		if($loginBad)
			echo '<br />' . $this->l('error_login_bad');
		if($passwordWrong)
			echo '<br />' . $this->l('error_password_wrong');
		echo '</div>';
	}
?>
<form action="/attach-handler.php" method="post" class="form-horizontal">
	<div class="control-group">
		<label class="control-label" for="uuid"><?php echo $this->l('field_uuid'); ?></label>
		<div class="controls">
			<?php
				if(!empty($uuid) && (!isset($uuidBad) || !$uuidBad)) {
					echo '<input type="text" class="span6" id="uuid" value="' . $uuid . '" disabled />';
					echo '<input type="hidden" name="uuid" value="' . $uuid . '" />';
				}
				else {
					echo '<input type="text" class="span6" id="uuid" name="uuid" maxlength="48" autocomplete="off" value="' . $uuid . '" required />';
					echo '<p class="muted"><small>' . $this->l('field_uuid_description') . '</small></p>';
				}
			?>
		</div>
	</div>
	<div class="control-group">
		<label class="control-label" for="login"><?php echo $this->l('field_login'); ?></label>
		<div class="controls">
			<input type="text" class="span6" id="login" name="login" maxlength="25" autocomplete="off" value="<?php echo $login; ?>" required />
			<p class="muted"><small><?php echo $this->l('field_login_description'); ?></small></p>
		</div>
	</div>
	<div class="control-group">
		<label class="control-label" for="password"><?php echo $this->l('field_password'); ?></label>
		<div class="controls">
			<input type="password" class="span6" id="password" name="password" autocomplete="off" required />
			<p class="muted"><small><?php echo $this->l('field_password_description'); ?></small></p>
		</div>
	</div>
	<div class="form-actions">
		<input type="submit" class="btn btn-primary" value="<?php echo $this->l('action_submit_form'); ?>" />
	</div>
</form>

<?php
	require(dirname(__FILE__) . '/includes/footer.php');
?>