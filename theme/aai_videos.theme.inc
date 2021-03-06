<?php

/**
 * Implements hook_theme().
 */
function aai_videos_theme_theme() {
  $path = drupal_get_path('module', 'aai_videos') .'/theme';
	$theme = array();
	$theme['aai_video_list'] = array(
    'arguments' => array('videos' => NULL),
    'file' => 'aai_videos.theme.inc',
    'path' => $path,
	);
	$theme['aai_video'] = array(
    'arguments' => array('video' => NULL),
    'file' => 'aai_videos.theme.inc',
    'path' => $path,
	);
	$theme['aai_video_top10'] = array(
    'arguments' => array('op' => NULL, 'videos' => NULL),
    'file' => 'aai_videos.theme.inc',
    'path' => $path,
	);
  return $theme;
}

/**
 * Theme function for video list display
 *
 * @param $videos
 *   An array containing videos values pulled from API.
 */
function theme_aai_video_list($videos){
	$output = '';
	if(is_array($videos)){
		$header = array(array('data' => t('Vote'), 'class'=>'vote-row'), array('data' => t('Title'), 'field' => 'title', 'sort' => 'asc'), array('data'=>t('Views'), 'class'=>'viewcount-row center'), array('data' => t('Added'), 'field' => 'added', 'sort' => 'asc', 'class'=>'added-row'));
		$rows = array();
		$allowed = aai_videos_is_allowed();
		
		// Sort order can only be ASC or DESC.
		$ts = tablesort_init($header);
    $sort = drupal_strtoupper($ts['sort']);
    $sort = in_array($sort, array('ASC', 'DESC')) ? $sort : '';
    switch($ts['sql']){
      case 'title':
      	$sort == 'DESC' ? usort($videos, "_aai_sort_name_desc") : usort($videos, "_aai_sort_name_asc");
      	break;
      case 'added':
    		if($sort == 'DESC') $videos = array_reverse($videos);
      	break;
    }
		foreach($videos as $video){
			$date = strtotime($video['created_at']);
			$row = array();
			
			$vote = '';
			$already_voted = aai_videos_limit_check($video['id'], 'vote');
			if($allowed) $vote .= '<div class="vote-minus vote-op">'.($already_voted ? '<span>-</span>' : l('-', 'videos/'.$video['id'].'/votedown', array('query'=>drupal_get_destination()))).'</div>';
			$vote .= '<div class="vote-count">'.$video['vote_tally'].'</div>';
			if($allowed) $vote .= '<div class="vote-plus vote-op">'.($already_voted ? '<span>+</span>' : l('+', 'videos/'.$video['id'].'/voteup', array('query'=>drupal_get_destination()))).'</div>';
			
			$row[] = array('data' => $vote, 'class'=>'vote-row');
			$row[] = l(htmlspecialchars_decode($video['title']), 'videos/'.$video['id'].'/view/'.$video['slug']);
			$row[] = array('data'=>$video['view_count'], 'class'=>'viewcount-row center');
			$row[] = array('data'=>format_date($date, 'custom', 'M j, Y'), 'class'=>'added-row');
			$rows[] = $row;
		}
		$output .= theme('table', $header, $rows);
	}
	return $output;
}

/**
 * Theme function for video list display
 *
 * @param $video
 *   An array containing video values pulled from API.
 */
function theme_aai_video($video){
	$output = '';
	if(is_array($video)){
		// We support YouTube and Vimeo videos
		if($details = _aai_video_extract($video['url'])){
			
			$vote = '<div class="node-video-vote clear-block">';
			$already_voted = aai_videos_limit_check($video['id'], 'vote');
			$vote .= '<div class="vote-desc">Vote: </div>';
			$vote .= '<div class="vote-minus vote-op">'.($already_voted ? '<span>-</span>' : l('-', 'videos/'.$video['id'].'/votedown', array('query'=>drupal_get_destination()))).'</div>';
			$vote .= '<div class="vote-count">'.$video['vote_tally'].'</div>';
			$vote .= '<div class="vote-plus vote-op">'.($already_voted ? '<span>+</span>' : l('+', 'videos/'.$video['id'].'/voteup', array('query'=>drupal_get_destination()))).'</div>';
			$vote .= '</div>';
		
			$date = strtotime($video['created_at']);
			$output .= '<div class="node-video node-video-'.$details['type'].'">';
			$output .= $vote;
			$output .= '<div class="node-video-added description">This video was added to our site on <strong>'.format_date($date, 'custom', 'M j, Y').'</strong></div>';
			$output .= '<div class="node-video-embed">'.$details['embed'].'</div>';
			$output .= '<div class="node-video-link">'.l($video['url'], $video['url'], array('attributes'=>array('class'=>'description'))).'</div>';
			$output .= '</div>';
		}else{
			drupal_goto($video['url']);
		}
	}
	return $output;
}

/**
 * Theme function for display of top 10 video by votes
 *
 * @param $op
 *   Should equal 'vote' or 'view'
 * @param $videos
 *   An array containing videos values pulled from API.
 */
function theme_aai_video_top10($op, $videos){
	$output = '';
	if(is_array($videos)){
		$field = $op == 'views' ? 'view_count' : 'vote_tally';
		$header = array('Video', ucfirst($op));
		$rows = array();
		
		// I believe there is an error in the API that calls for top videos by votes
		// It treats both negative and positive numbers as equal
		usort($videos, '_aai_sort_'.$op);
		
		foreach($videos as $video){
			$row = array();
			$row[] = l(htmlspecialchars_decode($video['title']), 'videos/'.$video['id'].'/view/'.$video['slug']);
			$row[] = array('data'=>$video[$field], 'class'=>'center');			
			$rows[] = $row;
		}
		
		$output .= theme('table',$header,$rows);
		
	}
	return $output;
}

/**
 * Take a video URL and return information about it
 *
 * @param $url
 *   The video URL
 * @return
 *   An array of headers to be used when sending a webform email. If headers
 *   for "From", "To", or "Subject" are set, they will take precedence over
 *   the values set in the webform configuration.
 */
function _aai_video_extract($url){
	$video_url = parse_url($url);
	if($video_url['host'] == 'www.youtube.com' || $video_url['host'] == 'youtube.com'){
		$array = explode("&", $video_url['query']);
		$id = substr($array[0], 2);
		$thumb = "http://img.youtube.com/vi/".$id."/0.jpg";
		return array(
			'type' => 'youtube',
			'id' => $id,
			'thumb' => $thumb,
			'embed' => '<iframe width="660" height="365" src="http://www.youtube.com/embed/'.$id.'" frameborder="0" allowfullscreen></iframe>'
		);
	} else if($video_url['host'] == 'www.vimeo.com' || $video_url['host'] == 'vimeo.com'){
		$id = substr($video_url['path'], 1);
		$hash = unserialize(file_get_contents("http://vimeo.com/api/v2/video/".$id.".php"));
		$thumb = $hash[0]["thumbnail_small"];
		return array(
			'type' => 'vimeo',
			'id' => $id,
			'thumb' => $thumb,
			'embed' => '<iframe src="http://player.vimeo.com/video/'.$id.'?title=0&amp;byline=0&amp;portrait=0&amp;color=668C3D" width="660" height="371" frameborder="0" webkitAllowFullScreen mozallowfullscreen allowFullScreen></iframe>'
		);
	}
	return FALSE;
}

/**
 * Helper function to sort alphabetically by name
 */
function _aai_sort_name_asc($a, $b){
	$c = strcmp($a['title'], $b['title']);
	if($c == 0) return 0;
  return ($c < 0) ? -1 : 1;
}

/**
 * Helper function to sort alphabetically by name
 */
function _aai_sort_name_desc($a, $b){
	$c = strcmp($a['title'], $b['title']);
	if($c == 0) return 0;
  return ($c > 0) ? -1 : 1;
}

/**
 * Helper function to sort by vote count
 */
function _aai_sort_votes($a, $b){
	if ($a['vote_tally'] == $b['vote_tally']) {
        return 0;
  }
  return ($a['vote_tally'] > $b['vote_tally']) ? -1 : 1;
}

/**
 * Helper function to sort by view count
 */
function _aai_sort_views($a, $b){
	if ($a['view_count'] == $b['view_count']) {
        return 0;
  }
  return ($a['view_count'] > $b['view_count']) ? -1 : 1;
}