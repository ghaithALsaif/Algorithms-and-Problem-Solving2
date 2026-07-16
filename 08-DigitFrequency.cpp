#include <iostream>
#include <string>
#include <cmath>
using namespace std;

int ReadFrequency(string message)
{
    int number;
    do
    {
        cout << message;
        cin >> number;
    } while (number < 0);
    return number;
}

string readpositive(string message)
{
    string order;
    do
    {
        cout << message;
        cin >> order;
    } while (stoi(order) <= 0);
    return order;
}
int digitfrequency(string order, int frequencynumber)

    
{
  int frequency = 0;
    for(int i = 0; i < order.length(); i++)
    {
        if((order[i]-'0')== frequencynumber)
        {
            frequency++;
        }
    }
    return frequency;

}

void printNumber(int frequency)
{
    cout << "\nFrequency of the digit: " << frequency << endl;
}



int main()
{

printNumber(digitfrequency(readpositive("Enter a positive number: "),ReadFrequency("Enter a digit to find its frequency: ") ));

 return 0;    
}



