drop database if exists boinc;
create database boinc;
use boinc;
SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";

-- --------------------------------------------------------

CREATE TABLE `host` (
  `id` int(11) NOT NULL,
  `create_time` int(11) NOT NULL,
  `userid` int(11) NOT NULL,
  `rpc_seqno` int(11) NOT NULL,
  `rpc_time` int(11) NOT NULL,
  `total_credit` double NOT NULL,
  `expavg_credit` double NOT NULL,
  `expavg_time` double NOT NULL,
  `timezone` int(11) NOT NULL,
  `domain_name` varchar(254) DEFAULT NULL,
  `serialnum` varchar(254) DEFAULT NULL,
  `last_ip_addr` varchar(254) DEFAULT NULL,
  `nsame_ip_addr` int(11) NOT NULL,
  `on_frac` double NOT NULL,
  `connected_frac` double NOT NULL,
  `active_frac` double NOT NULL,
  `cpu_efficiency` double NOT NULL,
  `duration_correction_factor` double NOT NULL,
  `p_ncpus` int(11) NOT NULL,
  `p_vendor` varchar(254) DEFAULT NULL,
  `p_model` varchar(254) DEFAULT NULL,
  `p_fpops` double NOT NULL,
  `p_iops` double NOT NULL,
  `p_membw` double NOT NULL,
  `os_name` varchar(254) DEFAULT NULL,
  `os_version` varchar(254) DEFAULT NULL,
  `m_nbytes` double NOT NULL,
  `m_cache` double NOT NULL,
  `m_swap` double NOT NULL,
  `d_total` double NOT NULL,
  `d_free` double NOT NULL,
  `d_boinc_used_total` double NOT NULL,
  `d_boinc_used_project` double NOT NULL,
  `d_boinc_max` double NOT NULL,
  `n_bwup` double NOT NULL,
  `n_bwdown` double NOT NULL,
  `credit_per_cpu_sec` double NOT NULL,
  `venue` varchar(254) NOT NULL,
  `nresults_today` int(11) NOT NULL,
  `avg_turnaround` double NOT NULL,
  `host_cpid` varchar(254) DEFAULT NULL,
  `external_ip_addr` varchar(254) DEFAULT NULL,
  `max_results_day` int(11) NOT NULL,
  `error_rate` double NOT NULL DEFAULT 0,
  `product_name` varchar(254) NOT NULL,
  `gpu_active_frac` double NOT NULL,
  `p_ngpus` int(11) NOT NULL,
  `p_gpu_fpops` double NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

CREATE TABLE `user` (
  `id` int(11) NOT NULL,
  `create_time` int(11) NOT NULL,
  `email_addr` varchar(254) NOT NULL,
  `name` varchar(254) DEFAULT NULL,
  `authenticator` varchar(254) DEFAULT NULL,
  `country` varchar(254) DEFAULT NULL,
  `postal_code` varchar(254) DEFAULT NULL,
  `total_credit` double NOT NULL,
  `expavg_credit` double NOT NULL,
  `expavg_time` double NOT NULL,
  `global_prefs` blob DEFAULT NULL,
  `project_prefs` blob DEFAULT NULL,
  `teamid` int(11) NOT NULL,
  `venue` varchar(254) NOT NULL,
  `url` varchar(254) DEFAULT NULL,
  `send_email` smallint(6) NOT NULL,
  `show_hosts` smallint(6) NOT NULL,
  `posts` smallint(6) NOT NULL,
  `seti_id` int(11) NOT NULL,
  `seti_nresults` int(11) NOT NULL,
  `seti_last_result_time` int(11) NOT NULL,
  `seti_total_cpu` double NOT NULL,
  `signature` varchar(254) DEFAULT NULL,
  `has_profile` smallint(6) NOT NULL,
  `cross_project_id` varchar(254) NOT NULL,
  `passwd_hash` varchar(254) NOT NULL,
  `email_validated` smallint(6) NOT NULL,
  `donated` smallint(6) NOT NULL,
  `login_token` char(32) NOT NULL DEFAULT '',
  `login_token_time` double NOT NULL DEFAULT 0,
  `previous_email_addr` varchar(254) NOT NULL DEFAULT '',
  `email_addr_change_time` double NOT NULL DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

ALTER TABLE `host`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT,
  ADD PRIMARY KEY (`id`),
  ADD KEY `host_userid_cpid` (`userid`,`host_cpid`),
  ADD KEY `host_domain_name` (`domain_name`),
  ADD KEY `host_avg` (`expavg_credit`),
  ADD KEY `host_tot` (`total_credit`),
  ADD KEY `create_time` (`create_time`);

-- --------------------------------------------------------

ALTER TABLE `user`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT,
  ADD PRIMARY KEY (`id`),
  ADD UNIQUE KEY `email_addr` (`email_addr`),
  ADD UNIQUE KEY `authenticator` (`authenticator`),
  ADD KEY `ind_tid` (`teamid`),
  ADD KEY `user_name` (`name`),
  ADD KEY `user_tot` (`total_credit`),
  ADD KEY `user_avg` (`expavg_credit`),
  ADD KEY `user_email_time` (`email_addr_change_time`),
  ADD KEY `create_time` (`create_time`);

-- --------------------------------------------------------

CREATE TABLE `result` (
  `id` int(11) NOT NULL,
  `create_time` int(11) NOT NULL,
  `workunitid` int(11) NOT NULL,
  `server_state` int(11) NOT NULL,
  `outcome` int(11) NOT NULL,
  `client_state` int(11) NOT NULL,
  `hostid` int(11) NOT NULL,
  `userid` int(11) NOT NULL,
  `report_deadline` int(11) NOT NULL,
  `sent_time` int(11) NOT NULL,
  `received_time` int(11) NOT NULL,
  `name` varchar(254) NOT NULL,
  `cpu_time` double NOT NULL,
  `xml_doc_in` blob DEFAULT NULL,
  `xml_doc_out` blob DEFAULT NULL,
  `stderr_out` blob DEFAULT NULL,
  `batch` int(11) NOT NULL,
  `file_delete_state` int(11) NOT NULL,
  `validate_state` int(11) NOT NULL,
  `claimed_credit` double NOT NULL,
  `granted_credit` double NOT NULL,
  `opaque` double NOT NULL,
  `random` int(11) NOT NULL,
  `app_version_num` int(11) NOT NULL,
  `appid` int(11) NOT NULL,
  `exit_status` int(11) NOT NULL,
  `teamid` int(11) NOT NULL,
  `priority` int(11) NOT NULL,
  `mod_time` timestamp NOT NULL DEFAULT current_timestamp() ON UPDATE current_timestamp(),
  `elapsed_time` double NOT NULL,
  `flops_estimate` double NOT NULL,
  `app_version_id` int(11) NOT NULL,
  `runtime_outlier` tinyint(4) NOT NULL,
  `size_class` smallint(6) NOT NULL DEFAULT -1,
  `peak_working_set_size` double NOT NULL,
  `peak_swap_size` double NOT NULL,
  `peak_disk_usage` double NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

ALTER TABLE `result`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT,
  ADD PRIMARY KEY (`id`),
  ADD UNIQUE KEY `res_name` (`name`),
  ADD KEY `res_wuid` (`workunitid`),
  ADD KEY `res_filedel` (`file_delete_state`),
  ADD KEY `res_userid_id` (`userid`,`id`),
  ADD KEY `res_userid_val` (`userid`,`validate_state`),
  ADD KEY `res_hostid_id` (`hostid`,`id`),
  ADD KEY `res_wu_user` (`workunitid`,`userid`),
  ADD KEY `res_host_state` (`hostid`,`server_state`),
  ADD KEY `recent_valid_apps` (`validate_state`,`appid`,`received_time`),
  ADD KEY `ind_res_batch_state` (`batch`,`server_state`,`outcome`,`client_state`,`validate_state`),
  ADD KEY `res_app_state` (`appid`,`server_state`,`outcome`,`validate_state`),
  ADD KEY `appid_xyz` (`app_version_id`,`server_state`,`received_time`),
  ADD KEY `ind_res_st_u` (`server_state`,`priority`,`appid`),
  ADD KEY `ind_res_st` (`appid`,`server_state`,`priority`);

-- --------------------------------------------------------

CREATE TABLE `workunit` (
  `id` int(11) NOT NULL,
  `create_time` int(11) NOT NULL,
  `appid` int(11) NOT NULL,
  `name` varchar(254) NOT NULL,
  `xml_doc` blob DEFAULT NULL,
  `batch` int(11) NOT NULL,
  `rsc_fpops_est` double NOT NULL,
  `rsc_fpops_bound` double NOT NULL,
  `rsc_memory_bound` double NOT NULL,
  `rsc_disk_bound` double NOT NULL,
  `need_validate` smallint(6) NOT NULL,
  `canonical_resultid` int(11) NOT NULL,
  `canonical_credit` double NOT NULL,
  `transition_time` int(11) NOT NULL,
  `delay_bound` int(11) NOT NULL,
  `error_mask` int(11) NOT NULL,
  `file_delete_state` int(11) NOT NULL,
  `assimilate_state` int(11) NOT NULL,
  `hr_class` int(11) NOT NULL,
  `opaque` double NOT NULL,
  `min_quorum` int(11) NOT NULL,
  `target_nresults` int(11) NOT NULL,
  `max_error_results` int(11) NOT NULL,
  `max_total_results` int(11) NOT NULL,
  `max_success_results` int(11) NOT NULL,
  `result_template_file` varchar(63) NOT NULL,
  `priority` int(11) NOT NULL,
  `mod_time` timestamp NOT NULL DEFAULT current_timestamp() ON UPDATE current_timestamp(),
  `rsc_bandwidth_bound` double NOT NULL,
  `fileset_id` int(11) NOT NULL,
  `app_version_id` int(11) NOT NULL,
  `transitioner_flags` tinyint(4) NOT NULL,
  `size_class` smallint(6) NOT NULL DEFAULT -1,
  `keywords` varchar(254) NOT NULL,
  `app_version_num` int(11) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

ALTER TABLE `workunit`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT,
  ADD PRIMARY KEY (`id`),
  ADD UNIQUE KEY `name` (`name`),
  ADD KEY `wu_val` (`appid`,`need_validate`),
  ADD KEY `wu_timeout` (`transition_time`),
  ADD KEY `wu_assim` (`appid`,`assimilate_state`),
  ADD KEY `wu_filedel_plus` (`file_delete_state`,`appid`,`mod_time`);

-- --------------------------------------------------------

CREATE TABLE `team` (
  `id` int(11) NOT NULL,
  `create_time` int(11) NOT NULL,
  `userid` int(11) NOT NULL,
  `name` varchar(254) NOT NULL,
  `name_lc` varchar(254) DEFAULT NULL,
  `url` varchar(254) DEFAULT NULL,
  `type` int(11) NOT NULL,
  `name_html` varchar(254) DEFAULT NULL,
  `description` blob DEFAULT NULL,
  `nusers` int(11) NOT NULL,
  `country` varchar(254) DEFAULT NULL,
  `total_credit` double NOT NULL DEFAULT 0,
  `expavg_credit` double NOT NULL DEFAULT 0,
  `expavg_time` double NOT NULL,
  `seti_id` int(11) NOT NULL DEFAULT 0,
  `ping_user` int(11) NOT NULL DEFAULT 0,
  `ping_time` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `joinable` tinyint(4) NOT NULL DEFAULT 1,
  `mod_time` timestamp NOT NULL DEFAULT current_timestamp() ON UPDATE current_timestamp()
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

ALTER TABLE `team`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=3940,
  ADD PRIMARY KEY (`id`),
  ADD UNIQUE KEY `name` (`name`),
  ADD KEY `team_avg` (`expavg_credit`),
  ADD KEY `team_tot` (`total_credit`),
  ADD KEY `team_userid` (`userid`),
	ADD FULLTEXT KEY `team_name` (`name`);

-- --------------------------------------------------------

--
-- Dump of Data
--

INSERT INTO `user` (`id`, `create_time`, `email_addr`, `name`, `authenticator`, `country`, `postal_code`, `total_credit`, `expavg_credit`, `expavg_time`, `global_prefs`, `project_prefs`, `teamid`, `venue`, `url`, `send_email`, `show_hosts`, `posts`, `seti_id`, `seti_nresults`, `seti_last_result_time`, `seti_total_cpu`, `signature`, `has_profile`, `cross_project_id`, `passwd_hash`, `email_validated`, `donated`, `login_token`, `login_token_time`, `previous_email_addr`, `email_addr_change_time`) VALUES
(1, 1549227110, 'tomasbrod@azet.sk', 'Tomáš Brada', '9befcbbd02d7aef09c2da422ab3f687e', 'Slovakia', '', 417174.6596907005, 0.361412, 1600567274.195138, 0x3c676c6f62616c5f707265666572656e6365733e0a3c6d6f645f74696d653e313535323538363734363c2f6d6f645f74696d653e0a3c6d61785f6e637075735f7063743e3130303c2f6d61785f6e637075735f7063743e0a3c6370755f75736167655f6c696d69743e3130303c2f6370755f75736167655f6c696d69743e0a3c72756e5f6f6e5f6261747465726965733e313c2f72756e5f6f6e5f6261747465726965733e0a3c72756e5f69665f757365725f6163746976653e313c2f72756e5f69665f757365725f6163746976653e0a3c72756e5f6770755f69665f757365725f6163746976653e313c2f72756e5f6770755f69665f757365725f6163746976653e0a3c69646c655f74696d655f746f5f72756e3e333c2f69646c655f74696d655f746f5f72756e3e0a3c73757370656e645f69665f6e6f5f726563656e745f696e7075743e303c2f73757370656e645f69665f6e6f5f726563656e745f696e7075743e0a3c73757370656e645f6370755f75736167653e303c2f73757370656e645f6370755f75736167653e0a3c776f726b5f6275665f6d696e5f646179733e313c2f776f726b5f6275665f6d696e5f646179733e0a3c776f726b5f6275665f6164646974696f6e616c5f646179733e333c2f776f726b5f6275665f6164646974696f6e616c5f646179733e0a3c6370755f7363686564756c696e675f706572696f645f6d696e757465733e3132303c2f6370755f7363686564756c696e675f706572696f645f6d696e757465733e0a3c6469736b5f696e74657276616c3e3630303c2f6469736b5f696e74657276616c3e0a3c6469736b5f6d61785f757365645f67623e3130303c2f6469736b5f6d61785f757365645f67623e0a3c6469736b5f6d696e5f667265655f67623e323c2f6469736b5f6d696e5f667265655f67623e0a3c6469736b5f6d61785f757365645f7063743e35303c2f6469736b5f6d61785f757365645f7063743e0a3c72616d5f6d61785f757365645f627573795f7063743e39303c2f72616d5f6d61785f757365645f627573795f7063743e0a3c72616d5f6d61785f757365645f69646c655f7063743e39303c2f72616d5f6d61785f757365645f69646c655f7063743e0a3c6c656176655f617070735f696e5f6d656d6f72793e313c2f6c656176655f617070735f696e5f6d656d6f72793e0a3c766d5f6d61785f757365645f7063743e37353c2f766d5f6d61785f757365645f7063743e0a3c6d61785f62797465735f7365635f646f776e3e303c2f6d61785f62797465735f7365635f646f776e3e0a3c6d61785f62797465735f7365635f75703e303c2f6d61785f62797465735f7365635f75703e0a3c6461696c795f786665725f6c696d69745f6d623e303c2f6461696c795f786665725f6c696d69745f6d623e0a3c6461696c795f786665725f706572696f645f646179733e303c2f6461696c795f786665725f706572696f645f646179733e0a3c646f6e745f7665726966795f696d616765733e303c2f646f6e745f7665726966795f696d616765733e0a3c636f6e6669726d5f6265666f72655f636f6e6e656374696e673e313c2f636f6e6669726d5f6265666f72655f636f6e6e656374696e673e0a3c68616e6775705f69665f6469616c65643e313c2f68616e6775705f69665f6469616c65643e0a3c2f676c6f62616c5f707265666572656e6365733e, 0x3c70726f6a6563745f707265666572656e6365733e0a3c7265736f757263655f73686172653e363030303c2f7265736f757263655f73686172653e0a3c6e6f5f6370753e303c2f6e6f5f6370753e0a3c616c6c6f775f626574615f776f726b3e313c2f616c6c6f775f626574615f776f726b3e0a3c70726f6a6563745f73706563696669633e0a3c617070735f73656c65637465643e0a3c6170705f69643e383c2f6170705f69643e0a3c6170705f69643e31303c2f6170705f69643e0a3c2f617070735f73656c65637465643e0a3c2f70726f6a6563745f73706563696669633e0a3c2f70726f6a6563745f707265666572656e6365733e, 3938, '', 'http://tbrada.eu/', 1, 1, 0, 0, 0, 0, 0, NULL, 0, '9befcbbd02d7aef09c2da422ab3f687e', '$2y$10$c40Nz4bmHhbFjDlnPZeTiOxHeR.jmSvJHsSanV02Y3E8FFXV0QBUG', 0, 0, '', 0, '', 0);

INSERT INTO `team` (`id`, `create_time`, `userid`, `name`, `name_lc`, `url`, `type`, `name_html`, `description`, `nusers`, `country`, `total_credit`, `expavg_credit`, `expavg_time`, `seti_id`, `ping_user`, `ping_time`, `joinable`, `mod_time`) VALUES
(3938, 1595538515, 1, 'Appreciation Club', 'appreciation club', '', 1, '', '', 0, 'Slovakia', 0, 0, 1595538515, 0, 0, 0, 1, '2020-07-23 21:08:35');

INSERT INTO `host` (`id`, `create_time`, `userid`, `rpc_seqno`, `rpc_time`, `total_credit`, `expavg_credit`, `expavg_time`, `timezone`, `domain_name`, `serialnum`, `last_ip_addr`, `nsame_ip_addr`, `on_frac`, `connected_frac`, `active_frac`, `cpu_efficiency`, `duration_correction_factor`, `p_ncpus`, `p_vendor`, `p_model`, `p_fpops`, `p_iops`, `p_membw`, `os_name`, `os_version`, `m_nbytes`, `m_cache`, `m_swap`, `d_total`, `d_free`, `d_boinc_used_total`, `d_boinc_used_project`, `d_boinc_max`, `n_bwup`, `n_bwdown`, `credit_per_cpu_sec`, `venue`, `nresults_today`, `avg_turnaround`, `host_cpid`, `external_ip_addr`, `max_results_day`, `error_rate`, `product_name`, `gpu_active_frac`, `p_ngpus`, `p_gpu_fpops`) VALUES
(1, 1549303684, 1, 1638, 1600596670, 352512.7132237, 0.094277, 1591886900.357685, 7200, 'manganp', '[BOINC|7.16.9]', '127.0.0.1', 1318, 0.432946, -1, 0.845627, 0, 1, 4, 'GenuineIntel', 'Intel(R) Core(TM) i5-5200U CPU @ 2.20GHz [Family 6 Model 61 Stepping 4]', 3876685752.32791, 11037250474.69266, 1000000000, 'Linux Arch', 'Arch Linux [5.8.9-arch2-1|libc 2.32 (GNU libc)]', 12469964800, 3145728, 0, 165356240896, 71449292800, 16248832, 0, 4225692478.119801, 35904.050947, 11041886.076041, 0, '', 0, 51505.4586945933, '70428fdf89250da8cabd061bb78a9c83', '147.232.187.63', 0, 0, '', 0.845627, 0, 0);

INSERT INTO `workunit` (`id`, `create_time`, `appid`, `name`, `xml_doc`, `batch`, `rsc_fpops_est`, `rsc_fpops_bound`, `rsc_memory_bound`, `rsc_disk_bound`, `need_validate`, `canonical_resultid`, `canonical_credit`, `transition_time`, `delay_bound`, `error_mask`, `file_delete_state`, `assimilate_state`, `hr_class`, `opaque`, `min_quorum`, `target_nresults`, `max_error_results`, `max_total_results`, `max_success_results`, `result_template_file`, `priority`, `mod_time`, `rsc_bandwidth_bound`, `fileset_id`, `app_version_id`, `transitioner_flags`, `size_class`, `keywords`, `app_version_num`) VALUES
(3426335, 1600624799, 7, 'tot5_51c_SrW9chTcyBXXsiF4GR31T5hXh', 0x3c66696c655f696e666f3e0a3c6e616d653e746f74355f3531635f537257396368546379425858736946344752333154356858682e696e3c2f6e616d653e0a3c75726c3e68747470733a2f2f626f696e632e7462726164612e65752f7462726164615f6367692f6675683f746f74355f3531635f537257396368546379425858736946344752333154356858682e696e3c2f75726c3e0a3c6d64355f636b73756d3e61663665343230353932623863386163383862316665636535356565376664383c2f6d64355f636b73756d3e0a3c6e62797465733e34333c2f6e62797465733e0a3c2f66696c655f696e666f3e0a3c776f726b756e69743e0a3c66696c655f7265663e0a3c66696c655f6e616d653e746f74355f3531635f537257396368546379425858736946344752333154356858682e696e3c2f66696c655f6e616d653e0a3c6f70656e5f6e616d653e696e7075742e6461743c2f6f70656e5f6e616d653e0a3c2f66696c655f7265663e0a3c2f776f726b756e69743e0a, 22, 40000000000000, 1e16, 100000000, 100000000, 0, 0, 0, 1601380268, 604800, 0, 0, 0, 0, 0, 1, 1, 8, 8, 1, 'templates/tot5_out', 2, '2020-09-22 10:52:48', 0, 0, 0, 0, -1, '', 0);

INSERT INTO `result` (`id`, `create_time`, `workunitid`, `server_state`, `outcome`, `client_state`, `hostid`, `userid`, `report_deadline`, `sent_time`, `received_time`, `name`, `cpu_time`, `xml_doc_in`, `xml_doc_out`, `stderr_out`, `batch`, `file_delete_state`, `validate_state`, `claimed_credit`, `granted_credit`, `opaque`, `random`, `app_version_num`, `appid`, `exit_status`, `teamid`, `priority`, `mod_time`, `elapsed_time`, `flops_estimate`, `app_version_id`, `runtime_outlier`, `size_class`, `peak_working_set_size`, `peak_swap_size`, `peak_disk_usage`) VALUES
(4032343, 1600748869, 3426335, 4, 0, 0, 1, 1, 1601380268, 1600771968, 0, 'tot5_51c_SrW9chTcyBXXsiF4GR31T5hXh_1', 0, 0x3c66696c655f696e666f3e0a20202020202020203c6e616d653e746f74355f3531635f537257396368546379425858736946344752333154356858685f315f72313535353031363837365f303c2f6e616d653e0a20202020202020203c67656e6572617465645f6c6f63616c6c792f3e0a20202020202020203c75706c6f61645f7768656e5f70726573656e742f3e0a20202020202020203c6d61785f6e62797465733e3132383030303030303c2f6d61785f6e62797465733e0a20202020202020203c75706c6f61645f75726c3e68747470733a2f2f626f696e632e7462726164612e65752f7462726164615f6367692f6675683f643c2f75706c6f61645f75726c3e0a202020203c786d6c5f7369676e61747572653e0a323131313530363763646438323530356163366234393235373161303664623535386439663731653536636335633735633433323235363731373962363062610a373433326435643730353633626233653136313166313163376663303062393439316130373463383233623362613930353263366232656134343230643564340a333237616464643162376162306331396438613837333334653966363334653665333761623232346162653731663462313866303365623061396535316562380a303861393365373734346633636639316237386537373139386337346662316230613038636264633962316632313962653830646164386637623938383732340a2e0a3c2f786d6c5f7369676e61747572653e0a3c2f66696c655f696e666f3e0a202020203c726573756c743e0a20202020202020203c66696c655f7265663e0a2020202020202020202020203c66696c655f6e616d653e746f74355f3531635f537257396368546379425858736946344752333154356858685f315f72313535353031363837365f303c2f66696c655f6e616d653e0a2020202020202020202020203c6f70656e5f6e616d653e6f75747075742e6461743c2f6f70656e5f6e616d653e0a20202020202020203c2f66696c655f7265663e0a202020203c2f726573756c743e, '', '', 22, 0, 0, 0, 0, 0, 787009339, 0, 7, 0, 0, 22, '2020-09-22 10:52:48', 0, 3418220690.712844, 45, 0, -1, 0, 0, 0);
