#include <windows.h>
#include <iostream>

using namespace std;

#define NUMBER_OF_PHILOSOPHERS 5
#define TIME_OF_WORK 1000 // час роботи програми
#define MAX_COUNT_OF_SEMAPHORE 1
#define MIN_EATING_TIME 20
#define MAX_EATING_TIME 80

bool isAlive = true; //прапор роботи програми. true == працює програма, false == кінець роботи

struct semaphore {// структура, що уособлює семафор
    HANDLE hObject;
    /*HANDLE - дескриптор
     * у Windows (і загалом у обчислювальній системі) дескриптор — це абстракція,
     * яка приховує реальну адресу пам'яті від користувача API,
     * дозволяючи системі прозоро реорганізувати фізичну пам'ять для програми.*/

    semaphore() {
        hObject = CreateSemaphore(// ініціалізація семафору
                nullptr, //атрибут безпеки
                MAX_COUNT_OF_SEMAPHORE,// початкове значення лічильника
                MAX_COUNT_OF_SEMAPHORE,// максимальне значення лічильника
                nullptr  // ім'я
        );
    }

    DWORD tryDecrement() {//спроба зменшення лічильника семафору
        return WaitForSingleObject(hObject, 0L);
    }

    void increment() {//збільшення лічильника семафору
        ReleaseSemaphore(hObject, 1, nullptr);
    }

    ~semaphore() {// деструктор
        CloseHandle(hObject);//закриття дескриптора
    }
};

semaphore *fork = new semaphore[NUMBER_OF_PHILOSOPHERS];//масив "вилок" філософів
semaphore *correctOutput = new semaphore();// семафор, що забезпечує коректний вивід у консоль

struct Philosopher {
    int id;
    semaphore *leftFork;
    semaphore *rightFork;

    Philosopher(int id) {
        this->id = id;
        int leftForkIndex = id;
        int rightForkIndex = (id + 1) % NUMBER_OF_PHILOSOPHERS;
        leftFork = &fork[leftForkIndex];
        rightFork = &fork[rightForkIndex];
    }

    void live() {
        // DWORD, або  "double word" є визначенням типу даних, характерним для Microsoft Windows
        //  У windows.h, dword — це "unsigned long int".
        DWORD waitLeftResult;
        DWORD waitRightResult;//змінні, які записують результат "взяття вилки"
        while (isAlive) {
            waitLeftResult = leftFork->tryDecrement();//спроба "взяти" ліву вилку
            if (waitLeftResult == WAIT_OBJECT_0) {
                waitRightResult = rightFork->tryDecrement();//спроба "взяти" праву вилку
                if (waitRightResult == WAIT_OBJECT_0) {//успішно взято дві вилки
                    eat();
                    rightFork->increment();
                }
                leftFork->increment();
            }
            think();
        }
    }

    void think() {
        while (correctOutput->tryDecrement() != WAIT_OBJECT_0);// цикл очікування
        // поки консоль не стане доступною для цього потоку
        cout << "Philosopher: " << id << " is thinking " << endl;
        correctOutput->increment();// передача консолі для іншого потоку
        Sleep(((rand() % (MAX_EATING_TIME - MIN_EATING_TIME + 1)) + MIN_EATING_TIME) / 2);
    }

    void eat() {
        while (correctOutput->tryDecrement() != WAIT_OBJECT_0);
        cout << "Philosopher: " << id << " has started eating " << endl;
        correctOutput->increment();
        Sleep((rand() % (MAX_EATING_TIME - MIN_EATING_TIME + 1)) + MIN_EATING_TIME);
        while (correctOutput->tryDecrement() != WAIT_OBJECT_0);
        cout << "Philosopher: " << id << " has finished eating " << endl;
        correctOutput->increment();
    }
};

DWORD WINAPI myThread(LPVOID lpParameter) {//функція з якої починається робота потоку
    Philosopher *philosopher = (Philosopher *) lpParameter;
    philosopher->live();
    delete philosopher;
    return 0;
}

int main(int argc, char *argv[]) {
    srand(time(0));
    HANDLE threads[NUMBER_OF_PHILOSOPHERS];
    for (int i = 0; i < NUMBER_OF_PHILOSOPHERS; ++i) {
        Philosopher *philosopher = new Philosopher(i);
        threads[i] = CreateThread(0,
                                  0,
                                  myThread,
                                  philosopher,
                                  0,
                                  NULL);
    }
    Sleep(TIME_OF_WORK);
    isAlive = false;
    WaitForMultipleObjects(NUMBER_OF_PHILOSOPHERS,  //кількість дескрипторів
                           threads,   // масив дескрипторів кожного потоку
                           TRUE,/*Якщо цей параметр має значення TRUE,
 * функція повертається, коли сигналізується стан усіх об’єктів у масиві threads*/
                           INFINITE);//час очікування об'єктів
    for (int i = 0; i < NUMBER_OF_PHILOSOPHERS; ++i) {
        CloseHandle(threads[i]);//закриття дескриптора потоку
    }
    delete[] fork;
    delete correctOutput;
    return 0;
}