#include <iostream>
#include <string>
#include <cmath>
using namespace std;


int readpositive(string message)
{
    int order;
    do
    {
        cout << message;
        cin >> order;
    } while (order <= 0);
    return order;
}



void printpattern(int order)
{
    
  
    for(char i ='A';i<=char(order+'A'-1);i++) 
    {
        for(char j='A';j<=i;j++)
        {
            cout <<i;
        }
        
        cout << endl;
    }


}


int main()
{

printpattern(readpositive("Enter a positive number: "));

    
 return 0;     
}



