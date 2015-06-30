/*
 * Original work Copyright (c) 2015, Alibaba Mobile Infrastructure (Android) Team
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.taobao.patch;


public class ReadWriteSet<E> {
	private static final Object[] EMPTY_ARRAY = new Object[0];
	
	private transient volatile Object[] elements = EMPTY_ARRAY;

	public synchronized boolean add(E e) {
		int index = indexOf(e);
		if (index >= 0)
			return false;

		Object[] newElements = new Object[elements.length + 1];
		System.arraycopy(elements, 0, newElements, 0, elements.length);
		newElements[elements.length] = e;
		elements = newElements;
		return true;
	}

	public synchronized boolean remove(E e) {
		int index = indexOf(e);
		if (index == -1)
			return false;

		Object[] newElements = new Object[elements.length - 1];
		System.arraycopy(elements, 0, newElements, 0, index);
		System.arraycopy(elements, index + 1, newElements, index, elements.length - index - 1);
		elements = newElements;
		return true;
	}

	private int indexOf(Object o) {
		for (int i = 0; i < elements.length; i++) {
			if (o.equals(elements[i]))
				return i;
		}
		return -1;
	}

	public Object[] getSnapshot() {
		return elements;
	}
	
	public int getSize() {
	    return elements.length;
	}
	
	public void clear() {
		elements = EMPTY_ARRAY;
	}
}
