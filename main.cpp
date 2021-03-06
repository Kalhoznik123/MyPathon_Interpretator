#include "lexer.h"
#include "parse.h"
#include "runtime.h"
#include "statement.h"
#include "test_runner_p.h"

#include <iostream>

using namespace std;

namespace parse {
void RunOpenLexerTests(TestRunner& tr);
}  // namespace parse

namespace ast {
void RunUnitTests(TestRunner& tr);
}
namespace runtime {
void RunObjectHolderTests(TestRunner& tr);
void RunObjectsTests(TestRunner& tr);
}  // namespace runtime

void TestParseProgram(TestRunner& tr);

namespace {

void RunMythonProgram(istream& input, ostream& output) {
    parse::Lexer lexer(input);
    auto program = ParseProgram(lexer);

    runtime::SimpleContext context{output};
    runtime::Closure closure;
    program->Execute(closure, context);
}

void TestSimplePrints() {
    istringstream input(R"(
print 57
print 10, 24, -8
print 'hello'
print "world"
print True, False
print
print None
)");

    ostringstream output;
    RunMythonProgram(input, output);

    ASSERT_EQUAL(output.str(), "57\n10 24 -8\nhello\nworld\nTrue False\n\nNone\n");
}

void TestAssignments() {
    istringstream input(R"(
x = 57
print x
x = 'C++ black belt'
print x
y = False
x = y
print x
x = None
print x, y
)");

    ostringstream output;
    RunMythonProgram(input, output);

    ASSERT_EQUAL(output.str(), "57\nC++ black belt\nFalse\nNone False\n");
}

void TestArithmetics() {
    istringstream input("print 1+2+3+4+5, 1*2*3*4*5, 1-2-3-4-5, 36/4/3, 2*5+10/2");

    ostringstream output;
    RunMythonProgram(input, output);

    ASSERT_EQUAL(output.str(), "15 120 -13 3 15\n");
}

void TestVariablesArePointers() {
    istringstream input(R"(
class Counter:
  def __init__():
    self.value = 0

  def add():
    self.value = self.value + 1

class Dummy:
  def do_add(counter):
    counter.add()

x = Counter()
y = x

x.add()
y.add()

print x.value

d = Dummy()
d.do_add(x)

print y.value
)");

    ostringstream output;
    RunMythonProgram(input, output);

    ASSERT_EQUAL(output.str(), "2\n3\n");
}
void TestMethodOverloading() {
       istringstream input1(R"(
class X:
  def f(a):
    print "one parameter overload"

  def f(a, b):
    print "two parameters overload"

x = X()
x.f(1)
)");

       istringstream input2(R"(
class X:
  def f(a):
    print "one parameter overload"

  def f(a, b):
    print "two parameters overload"

x = X()
x.f(1, 2)
)");

       ostringstream output;
       bool e1 = false;
       try {
           RunMythonProgram(input1, output);
       } catch (const std::runtime_error &) {
           e1 = true;
       }
       bool e2 = false;
       try {
           RunMythonProgram(input2, output);
       } catch (const std::runtime_error &) {
           e2 = true;
       }
       ASSERT(e1 == e2);
   }
void TextAssigment2(){
    istringstream input(R"(
class X:
  def __init__():
    self.value = 123

class Z:
  def spawn():
    return X()

z = Z()
a = z.spawn()
a.value = 456
b = z.spawn()
if a.value == 456:
  print "Success"
else:
  print "Failure", a.value
)");

    ostringstream output;
    RunMythonProgram(input, output);

    ASSERT_EQUAL(output.str(), "Success\n");
}
void TestBoolConversion() {
    istringstream input(R"(
a = 1
if a:
  print "truthy"
else:
  print "falsey"
)");

    ostringstream output;
    RunMythonProgram(input, output);
    auto a = output.str();
    std::cout << "Test" << std::endl;
}

void TestClass() {
    istringstream input(R"(
class A:
  def dummy():
    print "pass"
a = A()
print a.b.c
print "test"
)");

    ostringstream output;
    RunMythonProgram(input, output);
    auto a = output.str();
    std::cout << "Test" << std::endl;
}
void TestABC() {
    istringstream input(R"(
class A:
  def __init__():
    self.n = 0

class B:
  def __init__():
    self.a = A()

class C:
  def __init__():
    self.b = B()

c = C()
print c.b.a.n
)");

    ostringstream output;
    RunMythonProgram(input, output);
    ASSERT(output.str() == "0\n");
}

void TestABC2() {
    istringstream input(R"(
class A:
  def __init__():
    self.n = 0

class B:
  def __init__():
    self.not_a = 0

class C:
  def __init__():
    self.b = B()
    self.a = A()

c = C()
print c.b.a.n
)");

    ostringstream output;
    ASSERT_THROWS(RunMythonProgram(input, output), std::runtime_error);
}

void TestStringBoolConversion() {
    istringstream input(R"(
if "123":
  print "truthy"
else:
  print "falsey"
)");

    ostringstream output;
    RunMythonProgram(input, output);
    ASSERT(output.str() == "truthy\n");
}
void TestNoneBoolConversion() {
    istringstream input(R"(
if None:
  print "truthy"
else:
  print "falsey"
)");

    ostringstream output;
    RunMythonProgram(input, output);
    ASSERT(output.str() == "falsey\n");
    //ASSERT(output.str() == "truthy\n");
}
void TestBoolClassConversion() {
    istringstream input(R"(
class A:
  def __init__():
    self.n = 0
if A():
  print "truthy"
else:
  print "falsey"
)");

    ostringstream output;
    RunMythonProgram(input, output);
    ASSERT(output.str() == "truthy\n");
}

void TestNone() {
    istringstream input(R"(
class A:
  def __init__():
    self.n = 0
a = A()
a = None
if a:
  print "truthy"
else:
  print "falsey"
)");

    ostringstream output;
    RunMythonProgram(input, output);
    ASSERT(output.str() == "falsey\n");
}

void TestAll() {
    TestRunner tr;
    parse::RunOpenLexerTests(tr);
    runtime::RunObjectHolderTests(tr);
    runtime::RunObjectsTests(tr);
    ast::RunUnitTests(tr);
    TestParseProgram(tr);

    RUN_TEST(tr, TestSimplePrints);
    RUN_TEST(tr, TestAssignments);
    RUN_TEST(tr, TestArithmetics);
    RUN_TEST(tr, TestVariablesArePointers);
    RUN_TEST(tr, TestMethodOverloading);
    RUN_TEST(tr, TextAssigment2);
    RUN_TEST(tr, TestBoolConversion);
    RUN_TEST(tr,TestABC);
    RUN_TEST(tr,TestABC2);
    RUN_TEST(tr,TestStringBoolConversion);
    RUN_TEST(tr,TestNoneBoolConversion);
    RUN_TEST(tr,TestBoolClassConversion);
    RUN_TEST(tr,TestNone);
    RUN_TEST(tr, TestClass);
}

}  // namespace

int main() {
    try {
        TestAll();

        RunMythonProgram(cin, cout);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
		return 1;
    }
    return 0;
}
