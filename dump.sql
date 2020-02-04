SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";
-- --------------------------------------------------------

--
-- Структура таблицы `pid`
--

CREATE TABLE IF NOT EXISTS `pid` (
  `id` int(11) NOT NULL,
  `updated` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `value` float NOT NULL,
  `SET_VALUE` float NOT NULL,
  `power` float NOT NULL,
  `CONSTRAIN` float NOT NULL,
  `K_P` float NOT NULL,
  `K_I` float NOT NULL,
  `K_D` float NOT NULL,
  `set_value2` float NOT NULL,
  `DIFF_SUM` int(11) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Индексы сохранённых таблиц
--

--
-- Индексы таблицы `pid`
--
ALTER TABLE `pid`
  ADD PRIMARY KEY (`id`),
  ADD KEY `updated` (`updated`);

--
-- AUTO_INCREMENT для сохранённых таблиц
--

--
-- AUTO_INCREMENT для таблицы `pid`
--
ALTER TABLE `pid`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT;