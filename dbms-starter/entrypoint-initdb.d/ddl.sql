SET default_storage_engine = INNODB;

CREATE TABLE `e_store`.`brands`(
    `id` INT UNSIGNED NOT NULL auto_increment ,
    `name` VARCHAR(250) NOT NULL ,
    PRIMARY KEY(`id`)
);

CREATE TABLE `e_store`.`categories`(
    `id` INT UNSIGNED NOT NULL auto_increment ,
    `name` VARCHAR(250) NOT NULL ,
    PRIMARY KEY(`id`)
);

INSERT INTO `e_store`.`brands`(`name`)
VALUES
    ('Samsung');

INSERT INTO `e_store`.`brands`(`name`)
VALUES
    ('Nokia');

INSERT INTO `e_store`.`brands`(`name`)
VALUES
    ('Canon');
    
INSERT INTO `e_store`.`categories`(`name`)
VALUES
    ('Television');

INSERT INTO `e_store`.`categories`(`name`)
VALUES
    ('Mobile Phone');

INSERT INTO `e_store`.`categories`(`name`)
VALUES
    ('Camera');
    
    
CREATE TABLE `e_store`.`products`(
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT ,
    `name` VARCHAR(250) NOT NULL ,
    `brand_id` INT UNSIGNED NOT NULL ,
    `category_id` INT UNSIGNED NOT NULL ,
    `attributes` JSON,
    PRIMARY KEY(`id`) ,
    INDEX `CATEGORY_ID`(`category_id` ASC) ,
    INDEX `BRAND_ID`(`brand_id` ASC) ,
    CONSTRAINT `brand_id` FOREIGN KEY(`brand_id`) REFERENCES `e_store`.`brands`(`id`) ON DELETE RESTRICT ON UPDATE CASCADE ,
    CONSTRAINT `category_id` FOREIGN KEY(`category_id`) REFERENCES `e_store`.`categories`(`id`) ON DELETE RESTRICT ON UPDATE CASCADE
);


INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id` ,
    `attributes`
)
VALUES(
    'Prime' ,
    '1' ,
    '1' ,
    '{"screen": "50 inch", "resolution": "2048 x 1152 pixels", "ports": {"hdmi": 1, "usb": 3}, "speakers": {"left": "10 watt", "right": "10 watt"}}'
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id` ,
    `attributes`
)
VALUES(
    'Octoview' ,
    '1' ,
    '1' ,
    '{"screen": "40 inch", "resolution": "1920 x 1080 pixels", "ports": {"hdmi": 1, "usb": 2}, "speakers": {"left": "10 watt", "right": "10 watt"}}'
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id` ,
    `attributes`
)
VALUES(
    'Dreamer' ,
    '1' ,
    '1' ,
    '{"screen": "30 inch", "resolution": "1600 x 900 pixles", "ports": {"hdmi": 1, "usb": 1}, "speakers": {"left": "10 watt", "right": "10 watt"}}'
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id` ,
    `attributes`
)
VALUES(
    'Bravia' ,
    '1' ,
    '1' ,
    '{"screen": "25 inch", "resolution": "1366 x 768 pixels", "ports": {"hdmi": 1, "usb": 0}, "speakers": {"left": "5 watt", "right": "5 watt"}}'
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id` ,
    `attributes`
)
VALUES(
    'Proton' ,
    '1' ,
    '1' ,
    '{"screen": "20 inch", "resolution": "1280 x 720 pixels", "ports": {"hdmi": 0, "usb": 0}, "speakers": {"left": "5 watt", "right": "5 watt"}}'
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id` ,
    `attributes`
)
VALUES(
    'Desire' ,
    '2' ,
    '2' ,
    JSON_OBJECT(
        "network" ,
        JSON_ARRAY("GSM" , "CDMA" , "HSPA" , "EVDO") ,
        "body" ,
        "5.11 x 2.59 x 0.46 inches" ,
        "weight" ,
        "143 grams" ,
        "sim" ,
        "Micro-SIM" ,
        "display" ,
        "4.5 inches" ,
        "resolution" ,
        "720 x 1280 pixels" ,
        "os" ,
        "Android Jellybean v4.3"
    )
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id` ,
    `attributes`
)
VALUES(
    'Passion' ,
    '2' ,
    '2' ,
    JSON_OBJECT(
        "network" ,
        JSON_ARRAY("GSM" , "CDMA" , "HSPA") ,
        "body" ,
        "6.11 x 3.59 x 0.46 inches" ,
        "weight" ,
        "145 grams" ,
        "sim" ,
        "Micro-SIM" ,
        "display" ,
        "4.5 inches" ,
        "resolution" ,
        "720 x 1280 pixels" ,
        "os" ,
        "Android Jellybean v4.3"
    )
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id` ,
    `attributes`
)
VALUES(
    'Emotion' ,
    '2' ,
    '2' ,
    JSON_OBJECT(
        "network" ,
        JSON_ARRAY("GSM" , "CDMA" , "EVDO") ,
        "body" ,
        "5.50 x 2.50 x 0.50 inches" ,
        "weight" ,
        "125 grams" ,
        "sim" ,
        "Micro-SIM" ,
        "display" ,
        "5.00 inches" ,
        "resolution" ,
        "720 x 1280 pixels" ,
        "os" ,
        "Android KitKat v4.3"
    )
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id` ,
    `attributes`
)
VALUES(
    'Sensation' ,
    '2' ,
    '2' ,
    JSON_OBJECT(
        "network" ,
        JSON_ARRAY("GSM" , "HSPA" , "EVDO") ,
        "body" ,
        "4.00 x 2.00 x 0.75 inches" ,
        "weight" ,
        "150 grams" ,
        "sim" ,
        "Micro-SIM" ,
        "display" ,
        "3.5 inches" ,
        "resolution" ,
        "720 x 1280 pixels" ,
        "os" ,
        "Android Lollipop v4.3"
    )
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id` ,
    `attributes`
)
VALUES(
    'Joy' ,
    '2' ,
    '2' ,
    JSON_OBJECT(
        "network" ,
        JSON_ARRAY("CDMA" , "HSPA" , "EVDO") ,
        "body" ,
        "7.00 x 3.50 x 0.25 inches" ,
        "weight" ,
        "250 grams" ,
        "sim" ,
        "Micro-SIM" ,
        "display" ,
        "6.5 inches" ,
        "resolution" ,
        "1920 x 1080 pixels" ,
        "os" ,
        "Android Marshmallow v4.3"
    )
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id` ,
    `attributes`
)
VALUES(
    'Explorer' ,
    '3' ,
    '3' ,
    JSON_MERGE_PRESERVE(
        '{"sensor_type": "CMOS"}' ,
        '{"processor": "Digic DV III"}' ,
        '{"scanning_system": "progressive"}' ,
        '{"mount_type": "PL"}' ,
        '{"monitor_type": "LCD"}'
    )
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id` ,
    `attributes`
)
VALUES(
    'Runner' ,
    '3' ,
    '3' ,
    JSON_MERGE_PRESERVE(
        JSON_OBJECT("sensor_type" , "CMOS") ,
        JSON_OBJECT("processor" , "Digic DV II") ,
        JSON_OBJECT("scanning_system" , "progressive") ,
        JSON_OBJECT("mount_type" , "PL") ,
        JSON_OBJECT("monitor_type" , "LED")
    )
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id` ,
    `attributes`
)
VALUES(
    'Traveler' ,
    '3' ,
    '3' ,
    JSON_MERGE_PRESERVE(
        JSON_OBJECT("sensor_type" , "CMOS") ,
        '{"processor": "Digic DV II"}' ,
        '{"scanning_system": "progressive"}' ,
        '{"mount_type": "PL"}' ,
        '{"monitor_type": "LCD"}'
    )
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id` ,
    `attributes`
)
VALUES(
    'Walker' ,
    '3' ,
    '3' ,
    JSON_MERGE_PRESERVE(
        '{"sensor_type": "CMOS"}' ,
        '{"processor": "Digic DV I"}' ,
        '{"scanning_system": "progressive"}' ,
        '{"mount_type": "PL"}' ,
        '{"monitor_type": "LED"}'
    )
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id` ,
    `attributes`
)
VALUES(
    'Jumper' ,
    '3' ,
    '3' ,
    JSON_MERGE_PRESERVE(
        '{"sensor_type": "CMOS"}' ,
        '{"processor": "Digic DV I"}' ,
        '{"scanning_system": "progressive"}' ,
        '{"mount_type": "PL"}' ,
        '{"monitor_type": "LCD"}'
    )
);


# insert null
INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id` ,
    `attributes`
)
VALUES(
    'I am nu;;' ,
    '3' ,
    '3' ,
    NULL
);