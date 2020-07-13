#!/usr/bin/env php
<?php
// This file is part of BOINC.
// http://boinc.berkeley.edu
// Copyright (C) 2014 University of California
//
// BOINC is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// BOINC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with BOINC.  If not, see <http://www.gnu.org/licenses/>.

// Assign badges based on project total and per subproject credit.
// This code is mostly generic.
// You'll need to:
// - define your subproject in project/project.inc
// - use the <credit_by_app/> option in config.xml
// - supply your own project themed badge images
//  that have to follow a naming scheme (see below)
// See: http://boinc.berkeley.edu/trac/wiki/PerAppCredit

// For TBEG, Brod

require_once("../inc/util_ops.inc");


function get_badge_th($prefix, $ident, $value, $title, $descr)
{
    $name = "${prefix}_${ident}";
    $b = BoincBadge::lookup("name='$name'");
    if(!$b) {
        if(!$descr) $descr = $title.", $ident, >$value";
        $now = time();
        $id = BoincBadge::insert("(create_time, name, title, image_url, description) values ($now, '$name', '$title', 'img/$name.png', '$descr')");
        $b = BoincBadge::lookup_id($id);
        if (!$b) die("can't create badge $name\n");
    }
    if($title != $b->title) {
        echo "Warning: badge title mismatch on $name #$b->id, '$title' != '$b->title'\n";
    }
    $b->value  = $value;
    return $b;
}

// get the record for a badge (either total or subproject)
// badge_name_prefix should be user or team
// sub_project is an array with name and short_name as in $sub_projects
//
function get_sncredit_badges() {
    $badges = array();
    $badge_levels = array(
        0,0,0,0,0,0,0,0,
        /*6204*/ 0,
            37224,
           197600,
           785997,
          2629687,
          6873318,
        /*13610917*/0,
         16350666,
         0,
         84584914,
    );
    $limit = count($badge_levels);
    for ($i=0; $i < $limit; $i++) {
        if($badge_levels[$i]==0) continue;
        $lv = $i;
        $title = "Level $lv, over $badge_levels[$i] credits";
        $descr = "Earned over $badge_levels[$i] credits, which is the number of SNDLS on level $lv of rule 51.";
        $badges[] = get_badge_th('sncredit',$lv,$badge_levels[$i],$title,$descr);
    }
    return $badges;
}

function get_spt_badges() {
    $badges = array();
    $pf='badge_spt';
    $badges[] = get_badge_th($pf,'110k',110e3,"SPT, over 110k credits",0);
    $badges[] = get_badge_th($pf,'300k',300e3,"SPT, over 300k credits",0);
    $badges[] = get_badge_th($pf,'800k',800e3,"SPT, over 110k credits",0);
    $badges[] = get_badge_th($pf,'3M',3e6,"SPT, over 3M credits",0);
    //$badges[] = get_badge_th($pf,'12M',12e6,"SPT, over 12M credits",0);
    return $badges;
}

// decide which project total badge to assign, if any.
// Unassign other badges.
//
function assign_credit_badge($is_user, $item, $badges, $appids) {
    // count from highest to lowest level, so the user get's assigned the
    // highest possible level and the lower levels get removed
    //
    if($appids) {
        $credit = BoincCreditUser::sum('total',"where userid= ".$item->id." and appid in ($appids)");
    }
    else $credit = $item->total_credit;
    for ($i=count($badges)-1; $i>=0; $i--) {
        if ($credit >= $badges[$i]->value) {
            //echo "UT$item->id B{$badges[$i]->name} {$credit} >= {$badges[$i]->value}\n";
            assign_badge($is_user, $item, $badges[$i]);
            unassign_badges($is_user, $item, $badges, $i);
            return;
        }
    }
    // no level could be assigned so remove them all
    //
    unassign_badges($is_user, $item, $badges, -1);
}

// Scan through all the users/teams, 1000 at a time,
// and assign/unassign the badges (total and subproject)
//
function assign_all_badges($is_user)
{
    global $badge_levels;
    $kind = $is_user?"user":"team";

    // get badges
    //
    $badges = get_sncredit_badges();
    $spt_badges = get_spt_badges();

    $n = 0;
    $maxid = $is_user?BoincUser::max("id"):BoincTeam::max("id");
    while ($n <= $maxid) {
        $m = $n + 1000;
        if ($is_user) {
            $items = BoincUser::enum_fields("id, total_credit", "id>=$n and id<$m and total_credit>0");
        } else {
            $items = BoincTeam::enum_fields("id, total_credit", "id>=$n and id<$m and total_credit>0");
        }
        // for every user/team
        //
        foreach ($items as $item) {
            assign_credit_badge($is_user, $item, $badges, 0);
            assign_credit_badge($is_user, $item, $spt_badges, "10,11");
        }
        $n = $m;
    }
}

// one pass through DB for users
//
assign_all_badges(true);

// one pass through DB for teams
//
assign_all_badges(false);

?>
