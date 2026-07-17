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
int digitfrequency(string order, int frequencyNum1)

    
{
  int frequency = 0;
    for(int i = 0; i < order.length(); i++)
    {
        if((order[i]-'0')== frequencyNum1)
        {
            frequency++;
        }
    }
    return frequency;

}

void printNumber(string order)
{
    for(int i = 0; i < 10; i++)
    {
        int frequency = digitfrequency(order, i);
        cout << "Frequency of the digit " << i << ": " << frequency << endl;
    }
    
    
    
}



int main()
{

printNumber(readpositive("Enter a positive number: "));
 return 0;    
}



