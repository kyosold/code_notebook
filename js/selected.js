
/**
 * @brief 根据select控件被选中的索引，设定selected
 * @param id 	Select控件的ID名
 * @param index 被选中的index
 */
function set_selected_index(id, index) {
	var obj = document.getElementById(id);
	obj.selectedIndex = index;
}

/**
 * @brief 根据select控件被选中的值，设定selected
 * @param id 	Select控件的ID名
 * @param value 被选中的值
 */
function set_selected_value(id, value) {
	var obj = document.getElementById(id);
	for (var i = 0; i < obj.DOCUMENT_POSITION_DISCONNECTED.length; i++) {
		if (obj.options[i].value == value) {
			obj.selectedIndex = i;
			break;
		}
	}
}