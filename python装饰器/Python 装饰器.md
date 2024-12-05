# Python 装饰器

装饰器（decorators）是 Python 中的一种高级功能，它允许你动态地修改函数或类的行为。

装饰器是一种函数，它接受一个函数作为参数，并返回一个新的函数或修改原来的函数。

装饰器的语法使用 **@decorator_name** 来应用在函数或方法上。

Python 还提供了一些内置的装饰器，比如 **@staticmethod** 和 **@classmethod**，用于定义静态方法和类方法。

**装饰器的应用场景：**

- **日志记录**: 装饰器可用于记录函数的调用信息、参数和返回值。
- **性能分析**: 可以使用装饰器来测量函数的执行时间。
- **权限控制**: 装饰器可用于限制对某些函数的访问权限。
- **缓存**: 装饰器可用于实现函数结果的缓存，以提高性能。

### 基本语法

Python 装饰允许在不修改原有函数代码的基础上，动态地增加或修改函数的功能，装饰器本质上是一个接收函数作为输入并返回一个新的包装过后的函数的对象。

```python
def decorator_function(original_function):
    def wrapper(*args, **kwargs):
        # 这里是在调用原始函数前添加的新功能
        before_call_code()
        
        result = original_function(*args, **kwargs)
        
        # 这里是在调用原始函数后添加的新功能
        after_call_code()
        
        return result
    return wrapper

# 使用装饰器
@decorator_function
def target_function(arg1, arg2):
    pass  # 原始函数的实现
```

**解析：**decorator 是一个装饰器函数，它接受一个函数 func 作为参数，并返回一个内部函数 wrapper，在 wrapper 函数内部，你可以执行一些额外的操作，然后调用原始函数 func，并返回其结果。

- `decorator_function` 是装饰器，它接收一个函数 `original_function` 作为参数。
- `wrapper` 是内部函数，它是实际会被调用的新函数，它包裹了原始函数的调用，并在其前后增加了额外的行为。
- 当我们使用 `@decorator_function` 前缀在 `target_function` 定义前，Python会自动将 `target_function` 作为参数传递给 `decorator_function`，然后将返回的 `wrapper` 函数替换掉原来的 `target_function`。

### 使用装饰器

装饰器通过 **@** 符号应用在函数定义之前，例如：

```python
@time_logger
def target_function():
    pass
```

等同于：

```python
def target_function():
    pass
target_function = time_logger(target_function)
```

这会将 target_function 函数传递给 decorator 装饰器，并将返回的函数重新赋值给 target_function。从而，每次调用 target_function 时，实际上是调用了经过装饰器处理后的函数。

通过装饰器，开发者可以在保持代码整洁的同时，灵活且高效地扩展程序的功能。

### 带参数的装饰器

装饰器函数也可以接受参数，例如：

```python
def repeat(n):
    def decorator(func):
        def wrapper(*args, **kwargs):
            for _ in range(n):
                result = func(*args, **kwargs)
            return result
        return wrapper
    return decorator

@repeat(3)
def greet(name):
    print(f"Hello, {name}!")

greet("Alice")
```

以上代码中 repeat 函数是一个带参数的装饰器，它接受一个整数参数 n，然后返回一个装饰器函数。该装饰器函数内部定义了 wrapper 函数，在调用原始函数之前重复执行 n 次。因此，greet 函数在被 @repeat(3) 装饰后，会打印三次问候语。

### 类装饰器

在python中，其实也可以同类来实现装饰器的功能，称之为类装饰器。类装饰器的实现是调用了类里面的__call__函数。类装饰器的写法比我们装饰器函数的写法更加简单。

当我们将类作为一个装饰器，工作流程：

通过__init__（）方法初始化类
通过__call__（）方法调用真正的装饰方法

```python
import time

class BaiyuDecorator:
    def __init__(self, func):
        self.func = func
        print("执行类的__init__方法")

    def __call__(self, *args, **kwargs):
        print('进入__call__函数')
        t1 = time.time()
        self.func(*args, **kwargs)
        print("执行时间为：", time.time() - t1)

@BaiyuDecorator
def baiyu():
    print("----test----")
    time.sleep(2)

def python_blog_list():
    time.sleep(5)
    print('python_blog_list start')

@BaiyuDecorator
def blog(name):
    print('start blog')
    name()
    print('end blog')

if __name__ == '__main__':
    baiyu()
    print('--------------')
    blog(python_blog_list)
```



### 带参数的类装饰器

当装饰器有参数的时候，__init__() 函数就不能传入**func**（func代表要装饰的函数）了，而**func**是在__call__函数调用的时候传入的。

```python
class BaiyuDecorator:
    def __init__(self, arg1, arg2):  # init()方法里面的参数都是装饰器的参数
        print('执行类Decorator的__init__()方法')
        self.arg1 = arg1
        self.arg2 = arg2
 
    def __call__(self, func):  # 因为装饰器带了参数，所以接收传入函数变量的位置是这里
        print('执行类Decorator的__call__()方法')
 
        def baiyu_warp(*args):  # 这里装饰器的函数名字可以随便命名，只要跟return的函数名相同即可
            print('执行wrap()')
            print('装饰器参数：', self.arg1, self.arg2)
            print('执行' + func.__name__ + '()')
            func(*args)
            print(func.__name__ + '()执行完毕')
 
        return baiyu_warp
 
 
@BaiyuDecorator('Hello', 'Baiyu')
def example(a1, a2, a3):
    print('传入example()的参数：', a1, a2, a3)
 
 
if __name__ == '__main__':
    print('准备调用example()')
    example('Baiyu', 'Happy', 'Coder')
    print('测试代码执行完毕')
```

### 装饰器的顺序

```python
def BaiyuDecorator_1(func):
    def wrapper(*args, **kwargs):
        func(*args, **kwargs)
        print('我是装饰器1')
 
    return wrapper
 
def BaiyuDecorator_2(func):
    def wrapper(*args, **kwargs):
        func(*args, **kwargs)
        print('我是装饰器2')
 
    return wrapper
 
def BaiyuDecorator_3(func):
    def wrapper(*args, **kwargs):
        func(*args, **kwargs)
        print('我是装饰器3')
 
    return wrapper
 
@BaiyuDecorator_1
@BaiyuDecorator_2
@BaiyuDecorator_3
def baiyu():
    print("我是攻城狮白玉")
 
 
if __name__ == '__main__':
    baiyu()
```





参考:

https://blog.csdn.net/zhh763984017/article/details/120072425
[菜鸟](https://www.runoob.com/python3/python-decorators.html#:~:text=Python%20%E8%A3%85%E9%A5%B0%E5%99%A8%201%20%E6%97%A5%E5%BF%97%E8%AE%B0%E5%BD%95%3A%20%E8%A3%85%E9%A5%B0%E5%99%A8%E5%8F%AF%E7%94%A8%E4%BA%8E%E8%AE%B0%E5%BD%95%E5%87%BD%E6%95%B0%E7%9A%84%E8%B0%83%E7%94%A8%E4%BF%A1%E6%81%AF%E3%80%81%E5%8F%82%E6%95%B0%E5%92%8C%E8%BF%94%E5%9B%9E%E5%80%BC%E3%80%82%202%20%E6%80%A7%E8%83%BD%E5%88%86%E6%9E%90%3A,%E5%8F%AF%E4%BB%A5%E4%BD%BF%E7%94%A8%E8%A3%85%E9%A5%B0%E5%99%A8%E6%9D%A5%E6%B5%8B%E9%87%8F%E5%87%BD%E6%95%B0%E7%9A%84%E6%89%A7%E8%A1%8C%E6%97%B6%E9%97%B4%E3%80%82%203%20%E6%9D%83%E9%99%90%E6%8E%A7%E5%88%B6%3A%20%E8%A3%85%E9%A5%B0%E5%99%A8%E5%8F%AF%E7%94%A8%E4%BA%8E%E9%99%90%E5%88%B6%E5%AF%B9%E6%9F%90%E4%BA%9B%E5%87%BD%E6%95%B0%E7%9A%84%E8%AE%BF%E9%97%AE%E6%9D%83%E9%99%90%E3%80%82%204%20%E7%BC%93%E5%AD%98%3A%20%E8%A3%85%E9%A5%B0%E5%99%A8%E5%8F%AF%E7%94%A8%E4%BA%8E%E5%AE%9E%E7%8E%B0%E5%87%BD%E6%95%B0%E7%BB%93%E6%9E%9C%E7%9A%84%E7%BC%93%E5%AD%98%EF%BC%8C%E4%BB%A5%E6%8F%90%E9%AB%98%E6%80%A7%E8%83%BD%E3%80%82)