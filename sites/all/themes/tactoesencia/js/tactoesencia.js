(function ($) {
    $(document).ready(function(){
		//$('div.view-content').slick();
		$('.view-slider-front-multilenguage > .view-content').not('.slick-initialized').slick({
			infinite: true,
			dots: true,
			arrows: false,
			autoplay: true,
			autoplaySpeed: 4000,
			fade: true,
			fadeSpeed: 1000
           // arrows: true,
           // prevArrow:"<button type='button' class='glyphicon glyphicon-chevron-left'></button>",
           // nextArrow:"<button type='button' class='glyphicon glyphicon-chevron-right'></button>"
		});
	$('.view-masajes-multilanguage > .view-content >.views-row').each(function(index) {
		console.log(this.className); 
		
		$(this).find('.msj-allinfo').wrapAll('<div class="toginfo" />');
	});
	


	});
})(jQuery);

