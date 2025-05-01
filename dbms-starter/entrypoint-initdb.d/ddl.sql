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
    PRIMARY KEY(`id`) ,
    INDEX `CATEGORY_ID`(`category_id` ASC) ,
    INDEX `BRAND_ID`(`brand_id` ASC) ,
    CONSTRAINT `brand_id` FOREIGN KEY(`brand_id`) REFERENCES `e_store`.`brands`(`id`) ON DELETE RESTRICT ON UPDATE CASCADE ,
    CONSTRAINT `category_id` FOREIGN KEY(`category_id`) REFERENCES `e_store`.`categories`(`id`) ON DELETE RESTRICT ON UPDATE CASCADE
);


INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id`
)
VALUES(
    'Prime' ,
    '1' ,
    '1'
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id`
)
VALUES(
    'Octoview' ,
    '1' ,
    '1'
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id`
)
VALUES(
    'Dreamer' ,
    '1' ,
    '1'
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id`
)
VALUES(
    'Bravia' ,
    '1' ,
    '1'
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id`
)
VALUES(
    'Proton' ,
    '1' ,
    '1'
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id`
)
VALUES(
    'Desire' ,
    '2' ,
    '2'
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id`
)
VALUES(
    'Passion' ,
    '2' ,
    '2'
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id`
)
VALUES(
    'Emotion' ,
    '2' ,
    '2'
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id`
)
VALUES(
    'Sensation' ,
    '2' ,
    '2'
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id`
)
VALUES(
    'Joy' ,
    '2' ,
    '2'
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id`
)
VALUES(
    'Explorer' ,
    '3' ,
    '3'
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id`
)
VALUES(
    'Runner' ,
    '3' ,
    '3'
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id`
)
VALUES(
    'Traveler' ,
    '3' ,
    '3'
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id`
)
VALUES(
    'Walker' ,
    '3' ,
    '3'
);

INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id`
)
VALUES(
    'Jumper' ,
    '3' ,
    '3'
);


# insert null
INSERT INTO `e_store`.`products`(
    `name` ,
    `brand_id` ,
    `category_id`
)
VALUES(
    'I am nu;;' ,
    '3' ,
    '3'
);