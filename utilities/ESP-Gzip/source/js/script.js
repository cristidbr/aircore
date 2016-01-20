document.documentElement.addEventListener( 'click', function( e ){
	var cx = Math.round(( e.pageX / screen_pixels_wide ) * screen_mm_wide * 10 );
	var cy = Math.round(( e.pageY / screen_pixels_tall ) * screen_mm_tall * 10 );
});