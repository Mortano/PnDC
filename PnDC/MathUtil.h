#pragma once

namespace math
{
   
   template<typename T>
   constexpr bool IsEven(T val)
   {
      return !(val & 1);
   }

}