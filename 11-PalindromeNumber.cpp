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
string reverse(string order)
{
    string reversed = "";
    for (int i = order.length() - 1; i >= 0; i--)
    {
        reversed += order[i];
    }
    return reversed;
}

bool palindrome(string order)
{
    string reversed = reverse(order);
    if (order == reversed)
    {
        return true;
    }
    else
    {
        return false;
    }
}






void printNumber(string order)
{
    if (palindrome(order))
    {
        cout << order << " is a palindrome number." << endl;
    }
    else
    {
        cout << order << " is not a palindrome number." << endl;
    }
}

    




int main()
{

    printNumber(readpositive("Enter a positive number: "));

 return 0;    
}



