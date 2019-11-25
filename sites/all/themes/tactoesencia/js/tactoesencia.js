(function ($) {
    $(document).ready(function(){
		//$('div.view-content').slick();
		$('.slickPortada > .view-content').not('.slick-initialized').slick({
			autoplay:true,
            arrows: true,
            prevArrow:"<button type='button' class='glyphicon glyphicon-chevron-left'></button>",
            nextArrow:"<button type='button' class='glyphicon glyphicon-chevron-right'></button>"
		});
	



	});
})(jQuery);

