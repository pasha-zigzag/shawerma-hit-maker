#pragma once

// Иконки крутилок — те же SVG, что в макете Claude Design (viewBox 32×32).
namespace Icons
{
inline constexpr const char* meat = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" fill="none" stroke="#6E3418" stroke-width="1.5" stroke-linejoin="round" stroke-linecap="round"><path d="M12.5 19.5 L6 26" stroke-width="5.5"/><path d="M12.5 19.5 L6 26" stroke="#FFF8EC" stroke-width="2.6"/><circle cx="4.6" cy="25" r="2.3" fill="#FFF8EC"/><circle cx="7" cy="27.4" r="2.3" fill="#FFF8EC"/><path d="M11 18 C 7 12 11 4 18.5 4 C 25 4 29 9 28 14.5 C 27 20 20 23 14 21 Z" fill="#C2512E"/><path d="M16 8.5 C 18.5 7 22 7.2 24 9.5" stroke="#F3A07A"/><path d="M15 14 C 17 13 19.5 13.5 21 15" stroke="#8C3519"/></svg>)SVG";

inline constexpr const char* sauce = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" fill="none" stroke="#6E3418" stroke-width="1.5" stroke-linejoin="round" stroke-linecap="round"><path d="M13.5 2.5 H17.5 L18.5 7 H12.5 Z" fill="#E2321F"/><rect x="9.5" y="7" width="12" height="21" rx="4" fill="#FFF8EC"/><rect x="11.5" y="13" width="8" height="7" rx="1.5" fill="#F2C886"/><path d="M26 15.5 C 26 15.5 29 19 29 21 A 3 3 0 0 1 23 21 C 23 19 26 15.5 26 15.5 Z" fill="#FFF8EC"/></svg>)SVG";

inline constexpr const char* spice = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" fill="none" stroke="#6E3418" stroke-width="1.5" stroke-linejoin="round" stroke-linecap="round"><path d="M9.5 9 C 6 16 10.5 26.5 26.5 28.5 C 19 23.5 15.5 16.5 14.5 10 Z" fill="#E2321F"/><path d="M11.5 13 C 11.5 16.5 13 20 15.5 22.5" stroke="#FF9A8A"/><path d="M8.5 9.5 C 9.5 7 12.5 6.5 15 8.5 C 13 10.5 10.5 11 8.5 9.5 Z" fill="#3E9B4F"/><path d="M12 7.5 C 12 5 13.5 3 16 3" stroke="#3E9B4F" stroke-width="2"/></svg>)SVG";

inline constexpr const char* lavash = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" fill="none" stroke="#6E3418" stroke-width="1.5" stroke-linejoin="round" stroke-linecap="round"><g transform="rotate(-35 16 16)"><rect x="3" y="10" width="24" height="12" rx="6" fill="#F7DDA8"/><path d="M10 10.5 V21.5 M15 10.5 V21.5 M20 10.5 V21.5" stroke="#C9924F"/><ellipse cx="26" cy="16" rx="3.2" ry="6" fill="#C2512E"/><circle cx="25.5" cy="13.5" r="1.2" fill="#3E9B4F" stroke="none"/><circle cx="26.8" cy="17.5" r="1.1" fill="#FFF8EC" stroke="none"/></g></svg>)SVG";

inline constexpr const char* ambient = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" fill="none" stroke="#6E3418" stroke-width="1.5" stroke-linejoin="round" stroke-linecap="round"><path d="M9 22 H22.5 A5 5 0 0 0 22.5 12 A7 7 0 0 0 9.5 11 A5.5 5.5 0 0 0 9 22 Z" fill="#AFCBFF"/><path d="M24.5 2.5 L25.6 5.4 L28.5 6.5 L25.6 7.6 L24.5 10.5 L23.4 7.6 L20.5 6.5 L23.4 5.4 Z" fill="#FFF8EC"/><path d="M7 27 Q 9 25 11 27 T 15 27 T 19 27 T 23 27"/></svg>)SVG";

inline constexpr const char* lock = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 22 26" fill="none" stroke="#A9B3D6" stroke-width="2.4" stroke-linecap="round"><path d="M5 11V7a6 6 0 0 1 12 0v4"/><rect x="2" y="11" width="18" height="13" rx="3" fill="#A9B3D6" stroke="none"/></svg>)SVG";
} // namespace Icons
