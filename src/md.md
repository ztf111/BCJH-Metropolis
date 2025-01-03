我修改了一下run函数的定义，新增了一个 `recover_str`参数，返回的字典也增加了一个 `recover_str`的key：

```
run(data, rule, target_score, iterChef, iterRep, allowTool, **recover_str**, progressCallFunc)
```

用法：返回值的 `recover_str`记录了这次运行完之后的厨师、菜谱、厨具配置（但不包括加成）。如果在函数调用中传空字符串 `""`，那么就相当于之前的效果；如果传上一次返回的这个配置，那么下次运行就会由前一次结果初始化。而且这个是跨linux，emscripten一致的。

修改后的这个函数可以有几个好处：

1. 可以接着上一次的结果继续跑。如果之前跑了1000次发现太小想再跑1000次，原来的实现前1000次就白跑了，现在可以接着前面来，分数只高不低。
2. 我这边debug可以方便很多。之前没有留好方便debug的方式，如果之前只是用自然语言记录厨师菜谱分别是什么，只能一个一个配然后找bug很麻烦。其实本质上完全可以用想办法parse输出结果，但是要和c++配合比较复杂，我实在懒没做这个功能orz……这个字符串就比较方便，可以直接无缝导入程序，这样我复现问题就方便很多。
3. （也许）可以方便抄作业？如果服务器端存几个高分作业，那么用户直接导入就相当于从高分初始化可以比较快得到结果。或者也可以大家直接分享代码（函数实现里面遇到没有的厨师/菜谱，或者加成不同导致做不了菜谱，以及allowTool对不上的，还是随机初始化）。不过这个感觉用处也不是特别大，而且可能也比较麻烦。

因为需要改html，这次我就没有直接弄pr了。**我的仓库里默认分支的js还是旧的**，这是新的 [debug版wasm](https://github.com/hjenryin/BCJH-Metropolis-html/tree/debug-online/js)和[release版wasm](https://github.com/hjenryin/BCJH-Metropolis-html/tree/debug-online/js)。我从[debug分支](https://github.com/hjenryin/BCJH-Metropolis-html/tree/debug-online)部署了一个[预览](https://hjenryin.github.io/BCJH-Metropolis-html/debug)，在worker.js下面hard-code了一个 `recover_str`。如果在这个页面迭代次数都设0，那么每次结果都是一样的。

麻烦小鱼可以初始化一下这个值为空字符串，之后把这个返回值记下来传到下一次运行里吗？也可以设计一个刷新初始化机制，比方说刷新页面就重新设回空字符串什么的，怎样都行方便就好，防止这些结果卡在局部最优了。

有什么问题，或者我写出bug了，随时戳我！而且反正也不是修复错误也不急的，你什么时候方便看看都可以的。
