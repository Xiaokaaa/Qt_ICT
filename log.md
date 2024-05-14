- 在`transmissionwindow.cpp`中，代码
```c++
QRegularExpression regExp("^" + rule + "\\." + rule + "\\." + rule + "\\." + rule + "$");
    ui->targetIpLineEdit->setValidator(new QRegularExpressionValidator(regExp, this));
```
**报错**
```
E:\Qt_ICT\ICT_source_code\transmissionwindow.cpp:80: error: expected type-specifier before 'QRegExpValidator'
E:/Qt_ICT/ICT_source_code/transmissionwindow.cpp: In member function 'void TransmissionWindow::UiInit()':
E:/Qt_ICT/ICT_source_code/transmissionwindow.cpp:80:44: error: expected type-specifier before 'QRegExpValidator'
   80 |     ui->targetIpLineEdit->setValidator(new QRegExpValidator(QRegExp("^" + rule + "\\." + rule + "\\." + rule + "\\." + rule + "$"), this));
      |                                            ^~~~~~~~~~~~~~~~
```
**原因**：
`QRegExpValidator`是Qt框架的一部分，它自Qt 4.x版本开始就已经存在，并且继续存在于Qt 5.x版本中。在这些版本中，你可以使用`QRegExpValidator`来验证文本字段，如输入框中的数据是否符合特定的正则表达式模式。

从Qt 6.x版本开始，`QRegExp`类被标记为已弃用，Qt推荐使用`QRegularExpression`代替`QRegExp`。因此，对应的验证器也从`QRegExpValidator`变为`QRegularExpressionValidator`。这意味着如果你正在使用Qt 6.x或更高版本，应该考虑使用`QRegularExpressionValidator`。

所以在Qt 6.x中，我们需要将代码改为：
```cpp
QRegularExpression regExp("^" + rule + "\\." + rule + "\\." + rule + "\\." + rule + "$");
ui->targetIpLineEdit->setValidator(new QRegularExpressionValidator(regExp, this));
```

