
/**
 * @brief 根据指定的value，设定checked
 * @param name 	checkbox控件的name
 * @param value 需要被checked的值
 */
function set_checked_value(name, value) {
	var obj = document.getElementsByName(name);
	for (var i = 0; i < obj.length; i++) {
		if (obj[i].value == value) {
			obj[i].checked = true;
		}
	}
}