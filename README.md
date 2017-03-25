# localq-php
localq for php (client)
作为客户端，实现localq-server协议，并向localq-server推送数据

## Requirement
php5.4+

## Installation
```
$/path/to/phpize
$./configure --with-php-config=/path/to/php-config
$make && make install
```
## Usage
```php
$objLocalq = new Localq();
$data = array('hello');
$class = 'UserCls'; // 数据处理的类
$method = 'UserMtd'; // 数据处理的方法
if (-1 == $objLocalq->push(json_encode($data), $method, $class)) {
  // 出现错误 -1
  var_dump($objLocalq->getError());
}
```
