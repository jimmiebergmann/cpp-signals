# ++signals

#### Create signals and connect lambda or static functions.
```cpp
Signal<float, float> onMove;
onMove.connect([](float x, float y) {
    std::cout << "Moved to " << x << ", " << y << "." << std::endl;
});

onMove(10, 20);
onMove.emit(40, 50);
```
```
$ Moved to 10, 20.
$ Moved to 40, 50.
```

#### Add signals to classes, to internally or externally emit signals.
```cpp
class Text : public Slot
{
public:
    Text()
    {
        onChange.connect(this, &Text::printText);
    }
    void setText(const std::string & text)
    {
        m_text = text;
        onChange(m_text);
    }
    void printText(const std::string & text)
    {
        std::cout << "Text: " << text << std::endl;
    }
    Signal<const std::string &> onChange;
private:
    std::string m_text;
};

Text text;
text.setText("Hello world!");
text.onChange("Emit signal.");
```
```
$ Text: Hello world!
$ Text: Emit signal.
```

#### Interact between classes.
```cpp
class Person : public Slot
{
public:
    Person(const std::string & name) :
        m_name(name)
    {

    }
    void listen(const std::string & message)
    {
        std::cout << m_name << " heard \"" << message << "\"" << std::endl;
    }
    Signal<const std::string &> say;

private:
    std::string m_name;
};

Person linus("Linus");
Person marcus("Marcus");

linus.say.connect(&marcus, &Person::listen);
marcus.say.connect(&linus, &Person::listen);

linus.say("How are you?");
marcus.say("I'm fine.");
```
```
$ Marcus heard "How are you?"
$ Linus heard "I'm fine."
```

#### Different ways of connection deletion
##### Hint: All slot connections are deleted at class destruction, as demonstrated below.

```cpp
class Foo : public Slot
{
public:
    void bar(int) {}
};

int main()
{
    Foo * foobar = new Foo;
    Signal<int> signal;
    auto conn1 = signal.connect([](int) {});
    auto conn2 = signal.connect([](int) {});
    auto conn3 = signal.connect([](int) {});
    signal.connect(foobar, &Foo::bar);
    std::cout << "Currently " << signal.count() << " connections." << std::endl;
    
    conn1->disconnect();
    std::cout << "Currently " << signal.count() << " connections." << std::endl;

    signal.disconnect(conn2);
    std::cout << "Currently " << signal.count() << " connections." << std::endl;

    delete foobar;
    std::cout << "Currently " << signal.count() << " connections." << std::endl;

    signal.disconnectAll();
    std::cout << "Currently " << signal.count() << " connections." << std::endl; 
```
```
$ Currently 4 connections.
$ Currently 3 connections.
$ Currently 2 connections.
$ Currently 1 connections.
$ Currently 0 connections.
```



