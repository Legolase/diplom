SET default_storage_engine = INNODB;
CREATE TABLE `e_store`.`table` (
  `_id` BIGINT UNSIGNED NOT NULL auto_increment,
  `s_tinyint` TINYINT,
  `s_smallint` SMALLINT,
  `s_medium` MEDIUMINT,
  `s_int` INT,
  `s_bigint` BIGINT,
  `double` DOUBLE,
  `bool` BOOL,
  `small_varchar` VARCHAR(20),
  `big_varchar` VARCHAR(300),
  `char` CHAR(16),
  PRIMARY KEY(`_id`)
);
-- Вставка краевых значений
INSERT INTO `e_store`.`table` (
  `s_tinyint`, `s_smallint`, `s_medium`,
  `s_int`, `s_bigint`, `double`, `bool`,
  `small_varchar`, `big_varchar`, `char`
) VALUES
  (-128, -32768, -8388608, -2147483648, -9223372036854775808, -1.7e+308, 0, 'min', 'minimal', 'char_min'),
  (127, 32767, 8388607, 2147483647, 9223372036854775807, 1.7e+308, 1, 'max', 'maximal value test string', 'char_max'),
  (0, 0, 0, 0, 0, 0.0, 0, '', '', ''),
  (-1, -1, -1, -1, -1, 0.5, 1, 'neg1', 'negative 1', 'neg'),
  (64, 1024, 2048, 65536, 999999999, 9.9, 0, 'val', 'value mid', 'mid_val'),
  (100, 300, 500, 700, 900, 0.0000001, 1, 'a', 'b', 'c');

-- Обновление одной строки (например, второй)
UPDATE `e_store`.`table`
SET
  `bool` = 0,
  `small_varchar` = 'upd',
  `double` = 0.12345
WHERE `_id` = 6;

-- Удаление одной строки (например, первой)
DELETE FROM `e_store`.`table`
WHERE `_id` = 5;

-- Добавим ещё одну произвольную строку
INSERT INTO `e_store`.`table` (
  `s_tinyint`, `s_smallint`, `s_medium`,
  `s_int`, `s_bigint`, `double`, `bool`,
  `small_varchar`, `big_varchar`, `char`
) VALUES
  (42, 1024, 123456, 1000000, 1234567890123456, 2.718281828, 1, 'test', 'arbitrary data', 'test_string');