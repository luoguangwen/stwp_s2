
DELIMITER //
  CREATE PROCEDURE warning_to_policytask(
    IN t_uuid varchar(64),
    IN host_uuid varchar(64),    
    IN t_time datetime(0),
    IN p_uuid varchar(64),
    IN p_contents mediumtext,
    IN p_type int(11),
    IN p_name varchar(256),
    IN user varchar(64) )
    BEGIN
      INSERT INTO stwp_policies (uuid,name,type,contents,status,create_time,create_user_uuid) VALUES 
        (p_uuid,p_name,p_type,p_contents,1,t_time,(select uuid from stwp_users where account = user));
      INSERT INTO stwp_policies_task (uuid,host_uuid,policy_uuid,status,create_time,create_user_uuid) VALUES 
        (t_uuid,host_uuid,p_uuid, 0,t_time,(select uuid from stwp_users where account = user));
    END //
DELIMITER ;