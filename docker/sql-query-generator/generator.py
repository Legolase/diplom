import time
import random
import string
import mysql.connector

program_config = {
    'start_timeout': 20,
    'query_timeout': 5
}

# Настройки подключения к базе данных
config = {
    'user': 'root',
    'password': 'person',
    'host': 'dbms',
    'database': 'e_store',
    'raise_on_warnings': True
}


def create_table(cursor):
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS `e_store`.`table` (
            `_id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
            `tinyint_col` TINYINT,
            `int_col` INT,
            `double_col` DOUBLE,
            `varchar_col` VARCHAR(25),
            PRIMARY KEY (_id)
        ) ENGINE=InnoDB;
    """)


def random_string(length=10):
    return ''.join(random.choices(string.ascii_letters + string.digits, k=length))


def insert_row(cursor):
    tinyint_val = random.randint(-128, 127)
    int_val = random.randint(-100000, 100000)
    double_val = random.uniform(-1000.0, 1000.0)
    varchar_val = random_string(10)
    query = "INSERT INTO `e_store`.`table` (`tinyint_col`, `int_col`, `double_col`, `varchar_col`) VALUES ({}, {}, {}, '{}');".format(
            tinyint_val, int_val, double_val, varchar_val)
    print(query)
    cursor.execute(query)


def update_row(cursor):
    cursor.execute("SELECT _id FROM `table` ORDER BY RAND() LIMIT 1;")
    row = cursor.fetchone()
    if row:
        _id = row[0]
        new_tinyint_val = random.randint(-128, 127)
        new_int_val = random.randint(-100000, 100000)
        new_double_val = random.uniform(-1000.0, 1000.0)
        new_varchar_val = random_string(10)
        query = "UPDATE `e_store`.`table` SET `tinyint_col` = {}, `int_col` = {}, `double_col` = {}, `varchar_col` = '{}' WHERE `_id` = {};".format(
            new_tinyint_val, new_int_val, new_double_val, new_varchar_val, _id)
        print("Query: ", query)
        cursor.execute(query)


def delete_row(cursor):
    cursor.execute("SELECT _id FROM `table` ORDER BY RAND() LIMIT 1;")
    row = cursor.fetchone()
    if row:
        _id = row[0]
        query = "DELETE FROM `table` WHERE `_id` = {};".format(_id)
        print(query)
        cursor.execute(query)


def main():
    connection = mysql.connector.connect(**config)
    cursor = connection.cursor()
    create_table(cursor)
    connection.commit()

    try:
        while True:
            operation = random.choice(['insert', 'update', 'delete'])
            if operation == 'insert':
                insert_row(cursor)
            elif operation == 'update':
                update_row(cursor)
            elif operation == 'delete':
                delete_row(cursor)

            connection.commit()
            print(f"Performed {operation} operation.")
            time.sleep(program_config['query_timeout'])
    except KeyboardInterrupt:
        print("\nScript stopped by user.")
    finally:
        cursor.close()
        connection.close()


if __name__ == '__main__':
    print("Start generator in {} sec.".format(program_config['start_timeout']))
    time.sleep(program_config['start_timeout'])
    print("Started")
    main()
