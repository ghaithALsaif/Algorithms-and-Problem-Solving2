#include <iostream>
#include <string>
#include <cmath>
using namespace std;


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
int reverseNumber(string order)
{
    string reversenumber = "";
    for(int i = order.length() - 1; i >= 0; i--)
    {
      reversenumber += order[i];
    }
    return stoi(reversenumber);

}

void printNumber(int reversed_number)
{
    cout << "\nReversed number: " << reversed_number << endl;
}



int main()
{

printNumber(reverseNumber(readpositive("Enter a positive number: ")));

 return 0;    
}



