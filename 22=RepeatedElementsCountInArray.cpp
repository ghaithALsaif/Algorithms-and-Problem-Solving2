#include <iostream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
using namespace std;

/*int RandomGenarator(int min, int max)
{
    return rand() % (max - min + 1) + min;
}*/
int arr[100]; // Assuming a maximum size of 100
int Readnumber(string message)
{
    int num;
    cout << message;
    cin >> num;
    return num;
}
int arrayInput( int arrayLength)
{
    cout << "Enter the elements of the array: ";
    for (int i = 0; i < arrayLength; i++)
    {
        cout << "Element [" << i + 1 << "]: ";
        cin >> arr[i];
    }
    return arrayLength;
   
}
int CountRepeatedElements(int arrayLength, int RepeatedElement)
{
    int count = 0;
    for (int i = 0; i < arrayLength; i++)
    {
        if (arr[i] == RepeatedElement)
        {
            count++;
        }
    }
    return count;
}
    

void DisplayRepeatedElementsCount()
{
    
    int arrayLength = Readnumber("Enter the length of the array: ");
   arrayInput(arrayLength);
    
   int RepeatedElement = Readnumber("Enter the element to count its occurrences: ");
   cout<<"Original Array: ";
    for (int i = 0; i < arrayLength; i++)
    {
        cout << arr[i] << " ";
    }
    int count = CountRepeatedElements(arrayLength, RepeatedElement);
    cout << "\nThe element " << RepeatedElement << " appears " << count << " times in the array." << endl;
}



int main()
{
DisplayRepeatedElementsCount();
    
 return 0;
}



