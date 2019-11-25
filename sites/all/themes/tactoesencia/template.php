<?php

function tactoesencia_form_alter( &$form, &$form_state,$form_id ){
    if (isset($form['#node']->title)){
        $form['#node']->title = false;
    }
}

?>