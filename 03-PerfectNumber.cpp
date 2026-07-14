#include <iostream>
#include <string>
#include <cmath>
using namespace std;

int ReadNUmber(string message)
{
    int n;
    cout << message<<endl;
    cin >> n;
    return n;
}
bool Isperfectnumber(int n)
{
    int sum = 0;
    for(int i = 1; i<=n;i++)
    {
        if(sum==n)
        {
           
            return true;
      
        }
        else if(n%i==0)
        {
            sum+=i;
        }
        else if(sum>n)
        {
            
            return false;
    
        }


    }

    
}
void printresult(int num)
{
    if(Isperfectnumber(num))
    {
        cout << num << " is a perfect number" << endl;
    }
    else
    {
        cout << num << " is not a perfect number" << endl;
    }
}



int main()
{
printresult(ReadNUmber("Enter a number to check if it is a perfect number or not: "));

 return 0;    
}







