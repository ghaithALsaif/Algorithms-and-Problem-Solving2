#include <iostream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
using namespace std;

int RandomGenarator(int min, int max)
{
    return rand() % (max - min + 1) + min;
}
int arr[100]; // Assuming a maximum size of 100
int ReadPositiveInteger(string message )
{
    int num;
    do
    {
        cout << message;
        cin >> num;
    } while (num < 1 || num > 100);
    return num;
}
int fillArrayWithRandomNumbers( int size)
{
    for (int i = 0; i < size; i++)
    {
        arr[i] = RandomGenarator(1, 100);
    }
    return 0;
}
int MinOfArray(int size)
{
    int min = arr[0];
    for (int i = 1; i < size; i++)
    {
        if (arr[i] < min)
        {
            min = arr[i];
        }
    }
    return min;
}
void printArray(int size)
{
    
    fillArrayWithRandomNumbers(size);
    int min = MinOfArray(size);
    cout << "Array elements: ";
    for (int i = 0; i < size; i++)
    {
        cout << arr[i] << " ";
    }
    cout << endl;
    cout << "Minimum element: " << min << endl;
}


int main()
{
    srand((unsigned)time(NULL));

printArray(ReadPositiveInteger("Enter the size of the array (1-100): "));
    return 0;
}



