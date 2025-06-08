# Диплом - Change Data Capture

## Подготовка образов
```
./setup.bash
```

## Запуск
```
./dstart.bash
```

## Удаление **ВСЕХ** контейнеров
Если нет других запущенных контейнеров, то используйте:
```
./dstop-all.bash
```
Иначе ручками удаляйте:
```
sudo docker container stop cdc_container generator_container dbms_container
sudo docker container rm   cdc_container generator_container dbms_container
```

## Если всё плохо
Можете удалить **ВЕСЬ КЕШ** docker
```
./drcashe.bash
```
