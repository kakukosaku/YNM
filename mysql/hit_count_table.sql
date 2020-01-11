/*
记录网站点击次数
*/
CREATE TABLE hit_counter (
    slot TINYINT UNSIGNED NOT NULL PRIMARY KEY,
    cnt INT UNSIGNED NOT NULL
) ENGINE=InnoDB;

/*
每天点击次数
*/
CREATE TABLE daily_hit_counter (
    day DATE NOT NULL,
    slot TINYINT UNSIGNED NOT NULL,
    cnt INT UNSIGNED NOT NULL,
    PRIMARY KEY (day, slot)
) ENGINE=InnoDB;

/*
无需预告生成行sloat, 使用 `ON DUPLICATE KEY UPDATE`
*/
INSERT INTO daily_hit_counter(day, slot, cnt) VALUES(CURRENT_DATE, RAND() * 100, 1) ON DUPLICATE KEY UPDATE cnt = cnt + 1;

/*
定时聚合数据
*/
UPDATE daily_hit_counter AS c
    INNER JOIN (
        SELECT day, SUM(cnt) AS cnt, MIN(slot) AS mslot
        FROM daily_hit_counter
        GROUP BY day
    ) AS x USING(day)
SET c.cnt = IF(c.slot = x.mslot, x.cnt, 0),
    c.slot = IF(c.slot = x.mslot, 0, c.slot);

DELETE FROM daily_hit_counter WHERE slot <> 0 AND cnt = 0;
